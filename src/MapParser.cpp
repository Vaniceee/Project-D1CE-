#include "MapParser.h"
#include "AssetManager.h"
#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

    // ==================== МІНІ-DOM ПАРСЕР JSON (.tmj / .tsj) ====================

    struct JsonValue {
        enum class Type { Null, Bool, Number, String, Array, Object };

        using Array = std::vector<JsonValue>;
        using Object = std::unordered_map<std::string, JsonValue>;

        Type type = Type::Null;
        bool boolVal = false;
        double numVal = 0.0;
        std::string strVal;
        std::unique_ptr<Array> arrVal;
        std::unique_ptr<Object> objVal;

        const Array* AsArray() const {
            return (type == Type::Array && arrVal) ? arrVal.get() : nullptr;
        }

        const Object* AsObject() const {
            return (type == Type::Object && objVal) ? objVal.get() : nullptr;
        }

        const JsonValue* Get(const std::string& key) const {
            const Object* obj = AsObject();
            if (!obj) return nullptr;
            auto it = obj->find(key);
            return (it != obj->end()) ? &it->second : nullptr;
        }

        std::string GetString(const std::string& key, const std::string& def = "") const {
            const JsonValue* v = Get(key);
            return (v && v->type == Type::String) ? v->strVal : def;
        }

        bool GetBool(const std::string& key, bool def = false) const {
            const JsonValue* v = Get(key);
            return (v && v->type == Type::Bool) ? v->boolVal : def;
        }

        int GetInt(const std::string& key, int def = 0) const {
            const JsonValue* v = Get(key);
            if (!v) return def;
            if (v->type == Type::Number) return static_cast<int>(v->numVal);
            if (v->type == Type::String) {
                try { return std::stoi(v->strVal); }
                catch (...) { return def; }
            }
            return def;
        }

        uint32_t GetUInt32(const std::string& key, uint32_t def = 0) const {
            const JsonValue* v = Get(key);
            if (!v) return def;
            if (v->type == Type::Number && v->numVal >= 0.0) {
                return static_cast<uint32_t>(v->numVal);
            }
            if (v->type == Type::String) {
                try { return static_cast<uint32_t>(std::stoul(v->strVal)); }
                catch (...) { return def; }
            }
            return def;
        }

        float GetFloat(const std::string& key, float def = 0.0f) const {
            const JsonValue* v = Get(key);
            if (!v) return def;
            if (v->type == Type::Number) return static_cast<float>(v->numVal);
            if (v->type == Type::String) {
                try { return std::stof(v->strVal); }
                catch (...) { return def; }
            }
            return def;
        }
    };

    class JsonParser {
    public:
        explicit JsonParser(const std::string& src) : src_(src) {
            if (src_.size() >= 3 &&
                static_cast<unsigned char>(src_[0]) == 0xEF &&
                static_cast<unsigned char>(src_[1]) == 0xBB &&
                static_cast<unsigned char>(src_[2]) == 0xBF) {
                pos_ = 3;
            }
        }

        bool ParseRoot(JsonValue& outRoot) {
            SkipWs();
            if (pos_ >= src_.size()) return false;
            outRoot = ParseValue();
            return outRoot.type == JsonValue::Type::Object;
        }

        size_t Position() const { return pos_; }

    private:
        const std::string& src_;
        size_t pos_ = 0;

        void SkipWs() {
            while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }

        bool Match(char ch) {
            SkipWs();
            if (pos_ < src_.size() && src_[pos_] == ch) {
                ++pos_;
                return true;
            }
            return false;
        }

        JsonValue ParseValue() {
            SkipWs();
            if (pos_ >= src_.size()) return {};

            char c = src_[pos_];
            if (c == '{') return ParseObject();
            if (c == '[') return ParseArray();
            if (c == '"') {
                JsonValue v;
                v.type = JsonValue::Type::String;
                v.strVal = ParseString();
                return v;
            }
            if (c == 't' && src_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                JsonValue v;
                v.type = JsonValue::Type::Bool;
                v.boolVal = true;
                return v;
            }
            if (c == 'f' && src_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                JsonValue v;
                v.type = JsonValue::Type::Bool;
                v.boolVal = false;
                return v;
            }
            if (c == 'n' && src_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                return {};
            }
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                return ParseNumber();
            }

            ++pos_;
            return {};
        }

        std::string ParseString() {
            if (pos_ >= src_.size() || src_[pos_] != '"') return {};
            ++pos_;
            std::string out;
            while (pos_ < src_.size()) {
                char c = src_[pos_++];
                if (c == '"') break;
                if (c == '\\' && pos_ < src_.size()) {
                    char esc = src_[pos_++];
                    switch (esc) {
                    case '"':  out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/':  out.push_back('/'); break;
                    case 'n':  out.push_back('\n'); break;
                    case 'r':  out.push_back('\r'); break;
                    case 't':  out.push_back('\t'); break;
                    case 'u': {
                        size_t hexCount = 0;
                        while (pos_ < src_.size() && hexCount < 4 &&
                            std::isxdigit(static_cast<unsigned char>(src_[pos_]))) {
                            ++pos_;
                            ++hexCount;
                        }
                        out.push_back('?');
                        break;
                    }
                    default:   out.push_back(esc); break;
                    }
                }
                else {
                    out.push_back(c);
                }
            }
            return out;
        }

        JsonValue ParseNumber() {
            size_t start = pos_;
            if (src_[pos_] == '-') ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
            if (pos_ < src_.size() && src_[pos_] == '.') {
                ++pos_;
                while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
            }
            if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
                ++pos_;
                if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
                while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
            }

            JsonValue v;
            v.type = JsonValue::Type::Number;
            v.numVal = std::strtod(src_.substr(start, pos_ - start).c_str(), nullptr);
            return v;
        }

        JsonValue ParseArray() {
            JsonValue v;
            v.type = JsonValue::Type::Array;
            v.arrVal = std::make_unique<JsonValue::Array>();
            if (!Match('[')) return v;
            if (Match(']')) return v;

            while (pos_ < src_.size()) {
                v.arrVal->push_back(ParseValue());
                SkipWs();
                if (pos_ < src_.size() && src_[pos_] == ']') {
                    ++pos_;
                    break;
                }
                if (!Match(',')) break;
            }
            return v;
        }

        JsonValue ParseObject() {
            JsonValue v;
            v.type = JsonValue::Type::Object;
            v.objVal = std::make_unique<JsonValue::Object>();
            if (!Match('{')) return v;
            if (Match('}')) return v;

            while (pos_ < src_.size()) {
                SkipWs();
                if (pos_ >= src_.size() || src_[pos_] != '"') break;
                std::string key = ParseString();
                if (!Match(':')) break;
                (*v.objVal)[key] = ParseValue();
                SkipWs();
                if (pos_ < src_.size() && src_[pos_] == '}') {
                    ++pos_;
                    break;
                }
                if (!Match(',')) break;
            }
            return v;
        }
    };

    // ==================== ДОПОМІЖНІ ФУНКЦІЇ ====================

    std::string ToLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
            });
        return str;
    }

    bool LoadFileToString(const std::string& path, std::string& outStr) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        std::stringstream ss;
        ss << file.rdbuf();
        outStr = ss.str();
        return true;
    }

    std::string GetDirectory(const std::string& filePath) {
        size_t slash = filePath.find_last_of("/\\");
        return (slash != std::string::npos) ? filePath.substr(0, slash + 1) : "";
    }

    std::string NormalizePath(const std::string& rawPath) {
        std::string p = rawPath;
        std::replace(p.begin(), p.end(), '\\', '/');

        std::vector<std::string> parts;
        std::stringstream ss(p);
        std::string item;
        while (std::getline(ss, item, '/')) {
            if (item.empty() || item == ".") continue;
            if (item == "..") {
                if (!parts.empty() && parts.back() != "..") {
                    parts.pop_back();
                }
                else {
                    parts.push_back(item);
                }
            }
            else {
                parts.push_back(item);
            }
        }

        std::string res;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) res += "/";
            res += parts[i];
        }
        return res;
    }

    std::string ResolveAssetPath(const std::string& baseDir, const std::string& rawPath) {
        std::string clean = rawPath;
        std::replace(clean.begin(), clean.end(), '\\', '/');
        if (clean.rfind("assets/", 0) == 0) {
            return NormalizePath(clean);
        }
        return NormalizePath(baseDir + clean);
    }

    Texture2D LoadTextureSafe(const std::string& key, const std::string& resolvedPath) {
        Texture2D tex = AssetManager::Instance().GetTexture(key);
        if (tex.id != 0) return tex;

        AssetManager::Instance().LoadTexture(key, resolvedPath);
        return AssetManager::Instance().GetTexture(key);
    }

    VisualCategory DetectVisualCategory(const std::string& lowerLayerName) {
        if (lowerLayerName.rfind("floor_deco", 0) == 0 ||
            lowerLayerName.find("floor") != std::string::npos ||
            lowerLayerName.find("carpet") != std::string::npos ||
            lowerLayerName.find("rug") != std::string::npos) {
            return VisualCategory::FloorDeco;
        }
        if (lowerLayerName.rfind("small_deco", 0) == 0) {
            return VisualCategory::SmallDeco;
        }
        if (lowerLayerName.rfind("wall_deco", 0) == 0) {
            return VisualCategory::WallDeco;
        }
        // "furniture" або будь-який інший візуальний шар
        return VisualCategory::Furniture;
    }

    void ParseTilesetTiles(const JsonValue& tsObj, const std::string& tsDir, TilesetInfo& ts) {
        const JsonValue* tilesNode = tsObj.Get("tiles");
        const JsonValue::Array* tilesArr = tilesNode ? tilesNode->AsArray() : nullptr;
        if (!tilesArr) return;

        for (const auto& entry : *tilesArr) {
            if (!entry.AsObject()) continue;
            int localId = entry.GetInt("id", -1);
            if (localId < 0) continue;

            std::string rawImg = entry.GetString("image");
            if (!rawImg.empty()) {
                std::string key = ts.name + "_" + std::to_string(localId) + "_" + rawImg;
                std::string cleanPath = ResolveAssetPath(tsDir, rawImg);
                Texture2D tex = LoadTextureSafe(key, cleanPath);
                if (tex.id != 0) {
                    ts.individualTextures[localId] = tex;
                }
            }

            const JsonValue* og = entry.Get("objectgroup");
            const JsonValue* ogObjsNode = og ? og->Get("objects") : nullptr;
            const JsonValue::Array* ogObjs = ogObjsNode ? ogObjsNode->AsArray() : nullptr;
            if (ogObjs) {
                for (const auto& cObj : *ogObjs) {
                    float w = cObj.GetFloat("width");
                    float h = cObj.GetFloat("height");
                    if (w > 0.0f && h > 0.0f) {
                        ts.tileCollisions[localId].push_back({
                            cObj.GetFloat("x"),
                            cObj.GetFloat("y"),
                            w,
                            h
                            });
                    }
                }
            }
        }
    }

    void AddTileCollisionsToWorld(
        uint32_t rawGid,
        float worldTopLeftX,
        float worldTopLeftY,
        float targetWidth,
        float targetHeight,
        MapData& outData)
    {
        constexpr uint32_t FLIP_H = 0x80000000u;
        constexpr uint32_t FLIP_V = 0x40000000u;
        constexpr uint32_t FLIP_D = 0x20000000u;
        constexpr uint32_t ALL_FLAGS = FLIP_H | FLIP_V | FLIP_D;

        uint32_t cleanGid = rawGid & ~ALL_FLAGS;
        if (cleanGid == 0) return;

        const TilesetInfo* activeTs = nullptr;
        int localId = 0;
        for (int i = static_cast<int>(outData.tilesets.size()) - 1; i >= 0; --i) {
            if (cleanGid >= static_cast<uint32_t>(outData.tilesets[i].firstgid)) {
                activeTs = &outData.tilesets[i];
                localId = static_cast<int>(cleanGid) - outData.tilesets[i].firstgid;
                break;
            }
        }
        if (!activeTs) return;

        auto colIt = activeTs->tileCollisions.find(localId);
        if (colIt == activeTs->tileCollisions.end()) return;

        float origW = static_cast<float>(activeTs->tileWidth);
        float origH = static_cast<float>(activeTs->tileHeight);
        auto texIt = activeTs->individualTextures.find(localId);
        if (texIt != activeTs->individualTextures.end() && texIt->second.id != 0) {
            origW = static_cast<float>(texIt->second.width);
            origH = static_cast<float>(texIt->second.height);
        }

        float scaleX = (origW > 0.0f) ? (targetWidth / origW) : 1.0f;
        float scaleY = (origH > 0.0f) ? (targetHeight / origH) : 1.0f;
        bool flipH = (rawGid & FLIP_H) != 0;
        bool flipV = (rawGid & FLIP_V) != 0;

        for (const Rectangle& localCol : colIt->second) {
            float cw = localCol.width * scaleX;
            float ch = localCol.height * scaleY;
            float cx = localCol.x * scaleX;
            float cy = localCol.y * scaleY;

            if (flipH) cx = targetWidth - (cx + cw);
            if (flipV) cy = targetHeight - (cy + ch);

            outData.autoCollisionRects.push_back({
                worldTopLeftX + cx,
                worldTopLeftY + cy,
                cw,
                ch
                });
        }
    }

    void ParseTilesets(const JsonValue& root, const std::string& mapDir, const std::string& defaultTilesetKey, MapData& outData) {
        const JsonValue* tsNode = root.Get("tilesets");
        const JsonValue::Array* tsArray = tsNode ? tsNode->AsArray() : nullptr;

        if (tsArray) {
            for (const auto& entry : *tsArray) {
                if (!entry.AsObject()) continue;

                TilesetInfo ts;
                ts.firstgid = entry.GetInt("firstgid", 1);

                JsonValue ownedDoc;
                const JsonValue* effectiveObj = &entry;
                std::string effectiveDir = mapDir;

                std::string relSource = entry.GetString("source");
                if (!relSource.empty()) {
                    std::string fullTsPath = ResolveAssetPath(mapDir, relSource);
                    std::string tsText;

                    if (!LoadFileToString(fullTsPath, tsText) &&
                        fullTsPath.size() > 4 &&
                        fullTsPath.substr(fullTsPath.size() - 4) == ".tsx")
                    {
                        std::string altTsj = fullTsPath.substr(0, fullTsPath.size() - 4) + ".tsj";
                        if (LoadFileToString(altTsj, tsText)) {
                            fullTsPath = altTsj;
                        }
                    }

                    if (!tsText.empty()) {
                        JsonParser tsParser(tsText);
                        if (tsParser.ParseRoot(ownedDoc)) {
                            effectiveObj = &ownedDoc;
                            effectiveDir = GetDirectory(fullTsPath);
                        }
                        else {
                            GAME_WARN("MAP_PARSER", "Failed to parse external tileset JSON: %s", fullTsPath.c_str());
                        }
                    }
                    else {
                        GAME_WARN("MAP_PARSER", "External tileset not found (export as .tsj or embed it): %s", fullTsPath.c_str());
                    }
                }

                ts.name = effectiveObj->GetString("name", "tileset_" + std::to_string(ts.firstgid));
                int tw = effectiveObj->GetInt("tilewidth", 0);
                int th = effectiveObj->GetInt("tileheight", 0);
                ts.tileWidth = (tw > 0) ? tw : outData.tileWidth;
                ts.tileHeight = (th > 0) ? th : outData.tileHeight;

                ParseTilesetTiles(*effectiveObj, effectiveDir, ts);

                if (ts.individualTextures.empty()) {
                    std::string atlasImg = effectiveObj->GetString("image");
                    if (!atlasImg.empty()) {
                        std::string atlasPath = ResolveAssetPath(effectiveDir, atlasImg);
                        ts.texture = LoadTextureSafe(ts.name + "_atlas", atlasPath);
                    }
                    if (ts.texture.id == 0) {
                        ts.texture = AssetManager::Instance().GetTexture(ts.name);
                    }
                    if (ts.texture.id == 0 && !defaultTilesetKey.empty()) {
                        ts.texture = AssetManager::Instance().GetTexture(defaultTilesetKey);
                    }
                }

                outData.tilesets.push_back(std::move(ts));
            }
        }

        std::sort(outData.tilesets.begin(), outData.tilesets.end(),
            [](const TilesetInfo& a, const TilesetInfo& b) {
                return a.firstgid < b.firstgid;
            });

        if (outData.tilesets.empty() && !defaultTilesetKey.empty()) {
            TilesetInfo fallback;
            fallback.firstgid = 1;
            fallback.name = defaultTilesetKey;
            fallback.texture = AssetManager::Instance().GetTexture(defaultTilesetKey);
            fallback.tileWidth = outData.tileWidth;
            fallback.tileHeight = outData.tileHeight;
            outData.tilesets.push_back(std::move(fallback));
        }
    }

    void ParseSpawnObjects(const JsonValue::Array& objs, MapData& outData) {
        struct IndexedPoint { int order; Vector2 pos; };
        std::vector<IndexedPoint> indexedPatrol;
        int fallbackOrder = 1000;

        for (const auto& obj : objs) {
            if (!obj.AsObject()) continue;
            float x = obj.GetFloat("x");
            float y = obj.GetFloat("y");
            std::string spawnName = ToLower(obj.GetString("name"));

            if (spawnName == "player") {
                outData.playerSpawn = { x, y };
            }
            else if (spawnName == "guard") {
                outData.guardSpawns.push_back({ x, y });
            }
            else if (spawnName.rfind("patrol", 0) == 0) {
                int idx = fallbackOrder++;
                size_t us = spawnName.find('_');
                if (us != std::string::npos && us + 1 < spawnName.size()) {
                    try { idx = std::stoi(spawnName.substr(us + 1)); }
                    catch (...) {}
                }
                indexedPatrol.push_back({ idx,{ x, y } });
            }
        }

        if (!indexedPatrol.empty()) {
            std::sort(indexedPatrol.begin(), indexedPatrol.end(), [](const IndexedPoint& a, const IndexedPoint& b) {
                return a.order < b.order;
                });
            for (const auto& pt : indexedPatrol) {
                outData.guardPatrolPoints.push_back(pt.pos);
            }
        }
    }

    void ParseVisualObjects(const JsonValue::Array& objs, VisualCategory category, int layerOrder, int& globalSubOrder, MapData& outData) {
        for (const auto& obj : objs) {
            if (!obj.AsObject()) continue;
            if (!obj.GetBool("visible", true)) continue;

            uint32_t rawGid = obj.GetUInt32("gid", 0);
            if (rawGid == 0) continue;

            MapVisualObject vObj;
            vObj.rawGid = rawGid;
            vObj.x = obj.GetFloat("x");
            vObj.y = obj.GetFloat("y");
            vObj.width = obj.GetFloat("width");
            vObj.height = obj.GetFloat("height");
            vObj.category = category;
            vObj.layerOrder = layerOrder;
            vObj.subOrder = ++globalSubOrder;
            vObj.sortY = vObj.y;
            vObj.hasShadow = (category == VisualCategory::Furniture);

            outData.visualObjects.push_back(vObj);
        }
    }

    void ParseRectObjects(
        const JsonValue::Array& objs,
        std::vector<Rectangle>& rects,
        std::vector<std::string>* names,
        std::unordered_map<std::string, Rectangle>* targets,
        std::vector<InteractableObject>* interactables)
    {
        for (const auto& obj : objs) {
            if (!obj.AsObject()) continue;
            Rectangle rect = {
                obj.GetFloat("x"),
                obj.GetFloat("y"),
                obj.GetFloat("width"),
                obj.GetFloat("height")
            };
            std::string objName = obj.GetString("name", "unknown");

            if (targets && objName.size() > 2 && objName.substr(objName.size() - 2) == "_t") {
                (*targets)[objName] = rect;
            }
            else if (interactables && objName.size() > 2 && objName.substr(objName.size() - 2) == "_i") {
                InteractableObject item;
                item.fullId = objName.substr(0, objName.size() - 2);
                size_t us = item.fullId.find('_');
                item.type = (us != std::string::npos) ? item.fullId.substr(0, us) : item.fullId;
                item.bounds = rect;
                item.isUsed = false;
                interactables->push_back(std::move(item));
            }
            else {
                rects.push_back(rect);
                if (names) names->push_back(objName);
            }
        }
    }

    void ParseLayersRecursive(
        const JsonValue::Array& layersArr,
        int& visualLayerIndex,
        int& globalSubOrder,
        MapData& outData)
    {
        for (const auto& layer : layersArr) {
            if (!layer.AsObject()) continue;

            std::string layerType = layer.GetString("type");
            std::string layerName = layer.GetString("name");
            std::string lowerName = ToLower(layerName);
            bool isVisible = layer.GetBool("visible", true);

            if (layerType == "group") {
                const JsonValue* subLayersNode = layer.Get("layers");
                const JsonValue::Array* subLayersArr = subLayersNode ? subLayersNode->AsArray() : nullptr;
                if (subLayersArr) {
                    ParseLayersRecursive(*subLayersArr, visualLayerIndex, globalSubOrder, outData);
                }
                continue;
            }

            if (layerType == "tilelayer") {
                const JsonValue* dataNode = layer.Get("data");
                const JsonValue::Array* dataArr = dataNode ? dataNode->AsArray() : nullptr;
                if (!dataArr) continue;

                int layerW = layer.GetInt("width", outData.mapWidth);
                int layerH = layer.GetInt("height", outData.mapHeight);
                if (layerW <= 0) layerW = outData.mapWidth;
                if (layerH <= 0) layerH = outData.mapHeight;

                std::vector<int> layerData;
                layerData.reserve(dataArr->size());

                for (const auto& cell : *dataArr) {
                    uint32_t rawGid = 0;
                    if (cell.type == JsonValue::Type::Number && cell.numVal > 0.0) {
                        rawGid = static_cast<uint32_t>(cell.numVal);
                    }
                    layerData.push_back(static_cast<int>(rawGid));
                }

                for (int y = 0; y < layerH; ++y) {
                    for (int x = 0; x < layerW; ++x) {
                        int idx = x + y * layerW;
                        if (idx >= 0 && idx < static_cast<int>(layerData.size()) && layerData[idx] != 0) {
                            AddTileCollisionsToWorld(
                                static_cast<uint32_t>(layerData[idx]),
                                static_cast<float>(x * outData.tileWidth),
                                static_cast<float>(y * outData.tileHeight),
                                static_cast<float>(outData.tileWidth),
                                static_cast<float>(outData.tileHeight),
                                outData
                            );
                        }
                    }
                }

                if (!isVisible) continue;

                if (lowerName.find("roof") != std::string::npos ||
                    lowerName.find("above") != std::string::npos ||
                    lowerName.find("top") != std::string::npos)
                {
                    outData.abovePlayerLayers.push_back(std::move(layerData));
                }
                else {
                    outData.belowPlayerLayers.push_back(std::move(layerData));
                }
            }
            else if (layerType == "objectgroup") {
                const JsonValue* objsNode = layer.Get("objects");
                const JsonValue::Array* objsArr = objsNode ? objsNode->AsArray() : nullptr;
                if (!objsArr) continue;

                if (lowerName.find("colli") != std::string::npos)
                {
                    ParseRectObjects(*objsArr, outData.collisionRects, nullptr, nullptr, nullptr);
                }
                else if (lowerName.find("trig") != std::string::npos)
                {
                    ParseRectObjects(*objsArr, outData.triggers, &outData.triggerNames, &outData.teleportTargets, &outData.interactables);
                }
                else if (lowerName.find("spawn") != std::string::npos)
                {
                    ParseSpawnObjects(*objsArr, outData);
                }
                else if (isVisible) {
                    VisualCategory category = DetectVisualCategory(lowerName);
                    int order = (category == VisualCategory::FloorDeco) ? -1 : visualLayerIndex++;
                    ParseVisualObjects(*objsArr, category, order, globalSubOrder, outData);
                }
            }
        }
    }

    void ApplySupportSorting(MapData& outData) {
        for (auto& obj : outData.visualObjects) {
            switch (obj.category) {
            case VisualCategory::FloorDeco:
                obj.hasShadow = false;
                break;

            case VisualCategory::WallDeco:
                obj.sortY = obj.y;
                obj.hasShadow = false;
                break;

            case VisualCategory::Furniture:
                obj.sortY = obj.y;
                obj.hasShadow = true;
                AddTileCollisionsToWorld(
                    obj.rawGid,
                    obj.x,
                    obj.y - obj.height,
                    obj.width,
                    obj.height,
                    outData
                );
                break;

            case VisualCategory::SmallDeco: {
                const MapVisualObject* bestSupport = nullptr;
                float footX = obj.x + obj.width * 0.5f;
                float footY = obj.y;

                for (const auto& baseObj : outData.visualObjects) {
                    if (baseObj.category != VisualCategory::Furniture) continue;

                    if (footX >= baseObj.x && footX <= baseObj.x + baseObj.width &&
                        footY >= baseObj.y - baseObj.height && footY <= baseObj.y) {
                        if (!bestSupport ||
                            baseObj.sortY > bestSupport->sortY ||
                            (std::abs(baseObj.sortY - bestSupport->sortY) < 0.001f && baseObj.layerOrder > bestSupport->layerOrder)) {
                            bestSupport = &baseObj;
                        }
                    }
                }

                if (bestSupport) {
                    obj.sortY = bestSupport->sortY + 0.01f * static_cast<float>(obj.subOrder);
                    obj.hasShadow = false;
                }
                else {
                    obj.sortY = obj.y;
                    obj.hasShadow = true;
                    AddTileCollisionsToWorld(
                        obj.rawGid,
                        obj.x,
                        obj.y - obj.height,
                        obj.width,
                        obj.height,
                        outData
                    );
                }
                break;
            }
            }
        }
    }

} // namespace

// ==================== ПУБЛІЧНИЙ МЕТОД ПАРСЕРА ====================

bool MapParser::Parse(const std::string& mapPath, const std::string& defaultTilesetKey, MapData& outData) {
    std::string content;
    if (!LoadFileToString(mapPath, content)) {
        GAME_ERROR("MAP_PARSER", "Could not open map file: %s", mapPath.c_str());
        return false;
    }

    JsonParser parser(content);
    JsonValue root;
    if (!parser.ParseRoot(root)) {
        GAME_ERROR("MAP_PARSER", "Failed to parse JSON DOM for map: %s at pos %zu (bytes: %zu)",
            mapPath.c_str(), parser.Position(), content.size());
        return false;
    }

    outData.mapWidth = root.GetInt("width");
    outData.mapHeight = root.GetInt("height");
    outData.tileWidth = root.GetInt("tilewidth", 32);
    outData.tileHeight = root.GetInt("tileheight", 32);

    std::string mapDir = GetDirectory(mapPath);
    ParseTilesets(root, mapDir, defaultTilesetKey, outData);

    const JsonValue* layersNode = root.Get("layers");
    const JsonValue::Array* layersArr = layersNode ? layersNode->AsArray() : nullptr;
    if (!layersArr) {
        GAME_WARN("MAP_PARSER", "No 'layers' array found in %s (w=%d, h=%d)",
            mapPath.c_str(), outData.mapWidth, outData.mapHeight);
    }

    int visualLayerIndex = 0;
    int globalSubOrder = 0;
    if (layersArr) {
        ParseLayersRecursive(*layersArr, visualLayerIndex, globalSubOrder, outData);
    }

    ApplySupportSorting(outData);

    GAME_INFO("MAP_PARSER", "Map parsed: %s (Below layers: %zu, Above layers: %zu, Manual cols: %zu, Auto cols: %zu, Visuals: %zu, Patrol pts: %zu)",
        mapPath.c_str(),
        outData.belowPlayerLayers.size(),
        outData.abovePlayerLayers.size(),
        outData.collisionRects.size(),
        outData.autoCollisionRects.size(),
        outData.visualObjects.size(),
        outData.guardPatrolPoints.size());

    return true;
}