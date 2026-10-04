#include "StageBuilder.h"
#include "ArenaBoundary.h"
#include "ModelCache.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Camera.h"
#include "Skybox.h"
#include "MeshRenderer.h"
#include "MeshCollider.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// 読み込みには <fstream> ではなく DxLib の FileRead を使う
// <fstream> は中で <time.h> を読むが、Windows は大文字小文字を区別しないので
// インクルードパスにある Engine\Core\Time.h が先に見つかって壊れる

namespace {
    // Stage00.dat の中身
    // 先頭に件数が並び、キャラの初期位置、小物の配置、イベントの順に続く
    struct FileHeader {
        char magic[4];          // MAPD
        int32_t charaCount;
        int32_t objectCount;
        int32_t eventCount;
        float reserved[8];      // プレイヤーの初期位置など 使わない
    };

    struct CharaEntry {
        int32_t type;
        float position[3];
        float angle;
    };

    struct ObjectEntry {
        int32_t modelIndex;     // Stage_Obj000.mv1 の番号
        float position[3];
        float rotation[3];      // ラジアン
        float scale[3];
    };

    static_assert(sizeof(FileHeader) == 48, "Stage00.dat のヘッダーは 48 バイト");
    static_assert(sizeof(CharaEntry) == 20, "キャラの行は 20 バイト");
    static_assert(sizeof(ObjectEntry) == 40, "小物の行は 40 バイト");

    // この距離より外の小物は見た目だけにして、当たり判定を作らない
    // 判定は毎フレーム全部と比べるので、届かない場所のものまで持つと重くなる
    constexpr float COLLISION_RANGE = StageBuilder::ARENA_RADIUS + 600.0f;

    int g_terrainCollisionModel = -1;

    std::string MakeObjectPath(int index, bool isCollision) {
        char path[64];
        snprintf(path, sizeof(path), "Data/Stage/Stage_Obj%03d%s.mv1", index, isCollision ? "_c" : "");
        return path;
    }

    // Renderer が描くときと同じ 拡大 回転 移動 の順で組む
    MATRIX BuildMatrix(VECTOR position, const Quaternion& rotation, VECTOR scale) {
        MATRIX rotationMatrix = rotation.ToMatrix();
        MATRIX matrix = MGetIdent();
        for (int i = 0; i < 3; ++i) {
            matrix.m[0][i] = rotationMatrix.m[0][i] * scale.x;
            matrix.m[1][i] = rotationMatrix.m[1][i] * scale.y;
            matrix.m[2][i] = rotationMatrix.m[2][i] * scale.z;
        }
        matrix.m[3][0] = position.x;
        matrix.m[3][1] = position.y;
        matrix.m[3][2] = position.z;
        return matrix;
    }

    void PlaceObject(const ObjectEntry& entry) {
        VECTOR position = VGet(entry.position[0], entry.position[1], entry.position[2]);
        VECTOR scale = VGet(entry.scale[0], entry.scale[1], entry.scale[2]);
        Quaternion rotation = Quaternion::Euler(
            Transform::Rad2Deg(entry.rotation[0]),
            Transform::Rad2Deg(entry.rotation[1]),
            Transform::Rad2Deg(entry.rotation[2]));

        auto* object = Scene::Instance().CreateGameObject("StageObject");
        object->transform->localPosition = position;
        object->transform->localRotation = rotation;
        object->transform->localScale = scale;

        auto* renderer = object->AddComponent<MeshRenderer>();
        if (!renderer->LoadDuplicate(ModelCache::Get(MakeObjectPath(entry.modelIndex, false)))) return;

        VECTOR flat = VGet(position.x, 0.0f, position.z);
        if (VSize(flat) > COLLISION_RANGE) return;

        // 草のように判定用のモデルが無いものは、すり抜けられる飾りとして置く
        int collisionSource = ModelCache::Get(MakeObjectPath(entry.modelIndex, true));
        if (collisionSource == -1) return;

        auto* collider = object->AddComponent<MeshCollider>();
        if (collider->LoadDuplicate(collisionSource, BuildMatrix(position, rotation, scale))) {
            collider->Register();
        }
    }

    void PlaceObjects(const char* path) {
        int file = FileRead_open(path);
        if (file == 0) return;

        FileHeader header{};
        bool isValid = FileRead_read(&header, sizeof(header), file) != -1
            && std::memcmp(header.magic, "MAPD", 4) == 0;

        if (isValid) {
            // キャラの初期位置は使わない 敵はフェーズごとにその場で出す
            FileRead_seek(file, static_cast<LONGLONG>(sizeof(FileHeader) + sizeof(CharaEntry) * header.charaCount), SEEK_SET);

            for (int i = 0; i < header.objectCount; ++i) {
                ObjectEntry entry{};
                if (FileRead_read(&entry, sizeof(entry), file) == -1) break;
                PlaceObject(entry);
            }
        }

        FileRead_close(file);
    }
}

bool StageBuilder::Build(Camera* camera) {
    Scene& scene = Scene::Instance();

    auto* sky = scene.CreateGameObject("Sky");
    auto* skybox = sky->AddComponent<Skybox>();
    if (!skybox->Load("Data/Stage/Stage00_sky.mv1")) return false;
    if (camera) camera->SetSkybox(skybox);

    auto* terrain = scene.CreateGameObject("Terrain");
    auto* terrainRenderer = terrain->AddComponent<MeshRenderer>();
    if (!terrainRenderer->Load("Data/Stage/Stage00.mv1")) return false;

    // 当たり判定は見た目より粗いモデルを使う
    auto* terrainCollider = terrain->AddComponent<MeshCollider>();
    if (!terrainCollider->Load("Data/Stage/Stage00_c.mv1")) return false;
    terrainCollider->Register();
    g_terrainCollisionModel = terrainCollider->ModelHandle;

    PlaceObjects("Data/Stage/Stage00.dat");

    auto* boundary = scene.CreateGameObject("ArenaBoundary");
    auto* boundaryRenderer = boundary->AddComponent<ArenaBoundary>();
    boundaryRenderer->Setup(GetArenaCenter(), ARENA_RADIUS);

    return true;
}

bool StageBuilder::FindGroundHeight(float x, float z, float& outY) {
    if (g_terrainCollisionModel == -1) return false;

    VECTOR top = VGet(x, 5000.0f, z);
    VECTOR bottom = VGet(x, -5000.0f, z);
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(g_terrainCollisionModel, -1, top, bottom);
    if (!hit.HitFlag) return false;

    outY = hit.HitPosition.y;
    return true;
}

VECTOR StageBuilder::GetArenaCenter() {
    return VGet(0.0f, 0.0f, 0.0f);
}
