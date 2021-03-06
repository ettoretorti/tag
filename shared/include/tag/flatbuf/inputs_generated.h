// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_INPUTS_TAG_FLATBUF_H_
#define FLATBUFFERS_GENERATED_INPUTS_TAG_FLATBUF_H_

#include "flatbuffers/flatbuffers.h"

#include "common_generated.h"

namespace tag {
namespace flatbuf {

struct PlayerInput;

/// Player's input
struct PlayerInput FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_MOVEMENT = 4
  };
  /// The player's desired direction of movement.
  /// A length of 0 indicates no movement,
  /// while a length of 1 indicates movement at max speed.
  const tag::flatbuf::Vec2 *movement() const { return GetStruct<const tag::flatbuf::Vec2 *>(VT_MOVEMENT); }
  tag::flatbuf::Vec2 *mutable_movement() { return GetStruct<tag::flatbuf::Vec2 *>(VT_MOVEMENT); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<tag::flatbuf::Vec2>(verifier, VT_MOVEMENT) &&
           verifier.EndTable();
  }
};

struct PlayerInputBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_movement(const tag::flatbuf::Vec2 *movement) { fbb_.AddStruct(PlayerInput::VT_MOVEMENT, movement); }
  PlayerInputBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  PlayerInputBuilder &operator=(const PlayerInputBuilder &);
  flatbuffers::Offset<PlayerInput> Finish() {
    auto o = flatbuffers::Offset<PlayerInput>(fbb_.EndTable(start_, 1));
    return o;
  }
};

inline flatbuffers::Offset<PlayerInput> CreatePlayerInput(flatbuffers::FlatBufferBuilder &_fbb,
    const tag::flatbuf::Vec2 *movement = 0) {
  PlayerInputBuilder builder_(_fbb);
  builder_.add_movement(movement);
  return builder_.Finish();
}

}  // namespace flatbuf
}  // namespace tag

#endif  // FLATBUFFERS_GENERATED_INPUTS_TAG_FLATBUF_H_
