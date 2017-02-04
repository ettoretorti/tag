// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_MAP_TAG_FLATBUF_H_
#define FLATBUFFERS_GENERATED_MAP_TAG_FLATBUF_H_

#include "flatbuffers/flatbuffers.h"

#include "common_generated.h"

namespace tag {
namespace flatbuf {

struct ObstacleWColor;

struct Map;

struct MapRequest;

/// Represents an obstacle on a map
enum Obstacle {
  Obstacle_NONE = 0,
  Obstacle_Circle = 1,
  Obstacle_Polygon = 2,
  Obstacle_MIN = Obstacle_NONE,
  Obstacle_MAX = Obstacle_Polygon
};

inline const char **EnumNamesObstacle() {
  static const char *names[] = { "NONE", "Circle", "Polygon", nullptr };
  return names;
}

inline const char *EnumNameObstacle(Obstacle e) { return EnumNamesObstacle()[static_cast<int>(e)]; }

template<typename T> struct ObstacleTraits {
  static const Obstacle enum_value = Obstacle_NONE;
};

template<> struct ObstacleTraits<tag::flatbuf::Circle> {
  static const Obstacle enum_value = Obstacle_Circle;
};

template<> struct ObstacleTraits<tag::flatbuf::Polygon> {
  static const Obstacle enum_value = Obstacle_Polygon;
};

inline bool VerifyObstacle(flatbuffers::Verifier &verifier, const void *union_obj, Obstacle type);

/// An Obstacle combined with a Color
struct ObstacleWColor FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_OBSTACLE_TYPE = 4,
    VT_OBSTACLE = 6,
    VT_COLOR = 8
  };
  Obstacle obstacle_type() const { return static_cast<Obstacle>(GetField<uint8_t>(VT_OBSTACLE_TYPE, 0)); }
  bool mutate_obstacle_type(Obstacle _obstacle_type) { return SetField(VT_OBSTACLE_TYPE, static_cast<uint8_t>(_obstacle_type)); }
  /// The Obstacle
  const void *obstacle() const { return GetPointer<const void *>(VT_OBSTACLE); }
  void *mutable_obstacle() { return GetPointer<void *>(VT_OBSTACLE); }
  /// The color
  const tag::flatbuf::Color *color() const { return GetStruct<const tag::flatbuf::Color *>(VT_COLOR); }
  tag::flatbuf::Color *mutable_color() { return GetStruct<tag::flatbuf::Color *>(VT_COLOR); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_OBSTACLE_TYPE) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_OBSTACLE) &&
           VerifyObstacle(verifier, obstacle(), obstacle_type()) &&
           VerifyField<tag::flatbuf::Color>(verifier, VT_COLOR) &&
           verifier.EndTable();
  }
};

struct ObstacleWColorBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_obstacle_type(Obstacle obstacle_type) { fbb_.AddElement<uint8_t>(ObstacleWColor::VT_OBSTACLE_TYPE, static_cast<uint8_t>(obstacle_type), 0); }
  void add_obstacle(flatbuffers::Offset<void> obstacle) { fbb_.AddOffset(ObstacleWColor::VT_OBSTACLE, obstacle); }
  void add_color(const tag::flatbuf::Color *color) { fbb_.AddStruct(ObstacleWColor::VT_COLOR, color); }
  ObstacleWColorBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  ObstacleWColorBuilder &operator=(const ObstacleWColorBuilder &);
  flatbuffers::Offset<ObstacleWColor> Finish() {
    auto o = flatbuffers::Offset<ObstacleWColor>(fbb_.EndTable(start_, 3));
    return o;
  }
};

inline flatbuffers::Offset<ObstacleWColor> CreateObstacleWColor(flatbuffers::FlatBufferBuilder &_fbb,
    Obstacle obstacle_type = Obstacle_NONE,
    flatbuffers::Offset<void> obstacle = 0,
    const tag::flatbuf::Color *color = 0) {
  ObstacleWColorBuilder builder_(_fbb);
  builder_.add_color(color);
  builder_.add_obstacle(obstacle);
  builder_.add_obstacle_type(obstacle_type);
  return builder_.Finish();
}

/// Represents a map
struct Map FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_DIMENSIONS = 4,
    VT_BACKGROUND = 6,
    VT_OBSTACLES = 8
  };
  /// The width and height of the map
  const tag::flatbuf::Vec2 *dimensions() const { return GetStruct<const tag::flatbuf::Vec2 *>(VT_DIMENSIONS); }
  tag::flatbuf::Vec2 *mutable_dimensions() { return GetStruct<tag::flatbuf::Vec2 *>(VT_DIMENSIONS); }
  /// The background color
  const tag::flatbuf::Color *background() const { return GetStruct<const tag::flatbuf::Color *>(VT_BACKGROUND); }
  tag::flatbuf::Color *mutable_background() { return GetStruct<tag::flatbuf::Color *>(VT_BACKGROUND); }
  /// The obstacles
  const flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>> *obstacles() const { return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>> *>(VT_OBSTACLES); }
  flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>> *mutable_obstacles() { return GetPointer<flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>> *>(VT_OBSTACLES); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<tag::flatbuf::Vec2>(verifier, VT_DIMENSIONS) &&
           VerifyField<tag::flatbuf::Color>(verifier, VT_BACKGROUND) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_OBSTACLES) &&
           verifier.Verify(obstacles()) &&
           verifier.VerifyVectorOfTables(obstacles()) &&
           verifier.EndTable();
  }
};

struct MapBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_dimensions(const tag::flatbuf::Vec2 *dimensions) { fbb_.AddStruct(Map::VT_DIMENSIONS, dimensions); }
  void add_background(const tag::flatbuf::Color *background) { fbb_.AddStruct(Map::VT_BACKGROUND, background); }
  void add_obstacles(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>>> obstacles) { fbb_.AddOffset(Map::VT_OBSTACLES, obstacles); }
  MapBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  MapBuilder &operator=(const MapBuilder &);
  flatbuffers::Offset<Map> Finish() {
    auto o = flatbuffers::Offset<Map>(fbb_.EndTable(start_, 3));
    return o;
  }
};

inline flatbuffers::Offset<Map> CreateMap(flatbuffers::FlatBufferBuilder &_fbb,
    const tag::flatbuf::Vec2 *dimensions = 0,
    const tag::flatbuf::Color *background = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ObstacleWColor>>> obstacles = 0) {
  MapBuilder builder_(_fbb);
  builder_.add_obstacles(obstacles);
  builder_.add_background(background);
  builder_.add_dimensions(dimensions);
  return builder_.Finish();
}

inline flatbuffers::Offset<Map> CreateMapDirect(flatbuffers::FlatBufferBuilder &_fbb,
    const tag::flatbuf::Vec2 *dimensions = 0,
    const tag::flatbuf::Color *background = 0,
    const std::vector<flatbuffers::Offset<ObstacleWColor>> *obstacles = nullptr) {
  return CreateMap(_fbb, dimensions, background, obstacles ? _fbb.CreateVector<flatbuffers::Offset<ObstacleWColor>>(*obstacles) : 0);
}

/// Used to request the map from a server
struct MapRequest FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           verifier.EndTable();
  }
};

struct MapRequestBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  MapRequestBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  MapRequestBuilder &operator=(const MapRequestBuilder &);
  flatbuffers::Offset<MapRequest> Finish() {
    auto o = flatbuffers::Offset<MapRequest>(fbb_.EndTable(start_, 0));
    return o;
  }
};

inline flatbuffers::Offset<MapRequest> CreateMapRequest(flatbuffers::FlatBufferBuilder &_fbb) {
  MapRequestBuilder builder_(_fbb);
  return builder_.Finish();
}

inline bool VerifyObstacle(flatbuffers::Verifier &verifier, const void *union_obj, Obstacle type) {
  switch (type) {
    case Obstacle_NONE: return true;
    case Obstacle_Circle: return verifier.VerifyTable(reinterpret_cast<const tag::flatbuf::Circle *>(union_obj));
    case Obstacle_Polygon: return verifier.VerifyTable(reinterpret_cast<const tag::flatbuf::Polygon *>(union_obj));
    default: return false;
  }
}

}  // namespace flatbuf
}  // namespace tag

#endif  // FLATBUFFERS_GENERATED_MAP_TAG_FLATBUF_H_
