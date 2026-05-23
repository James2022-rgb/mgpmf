#include "mgpmf/public/gpmf_sample.h"

// c++ system headers -----------------------------------
#include <cassert>
#include <cstring>

// external headers -------------------------------------
#include "jgpmf_capi.h"

namespace mgpmf {

namespace {

inline Vec3f ToVec3f(JgpmfVec3f const& v) {
  return Vec3f{ v.x, v.y, v.z };
}
inline Quatf ToQuatf(JgpmfQuatf const& q) {
  return Quatf{ q.w, q.x, q.y, q.z };
}

} // namespace

struct GpmfSample::Impl final {
  JgpmfSample* handle = nullptr;

  ~Impl() {
    if (handle != nullptr) {
      jgpmf_sample_free(handle);
      handle = nullptr;
    }
  }
};

std::unique_ptr<GpmfSample> GpmfSample::Parse(uint8_t const* bytes, uint32_t len) {
  if (bytes == nullptr || len == 0u) return nullptr;

  JgpmfSample* h = jgpmf_sample_parse(bytes, static_cast<size_t>(len));
  if (h == nullptr) return nullptr;

  auto sample = std::unique_ptr<GpmfSample>(new GpmfSample());
  sample->impl_->handle = h;
  return sample;
}

GpmfSample::GpmfSample() : impl_(std::make_unique<Impl>()) {}
GpmfSample::~GpmfSample() = default;

std::optional<Gps9> GpmfSample::gps9() const {
  JgpmfGps9 raw{};
  if (!jgpmf_sample_gps9(impl_->handle, &raw)) return std::nullopt;

  Gps9 out{};
  out.fix                    = raw.fix;
  out.dop                    = raw.dop;
  out.latitude               = raw.latitude;
  out.longitude              = raw.longitude;
  out.altitude               = raw.altitude;
  out.speed_2d               = raw.speed_2d;
  out.speed_3d               = raw.speed_3d;
  out.days_since_2000        = raw.days_since_2000;
  out.seconds_since_midnight = raw.seconds_since_midnight;
  return out;
}

std::span<Vec3f const> GpmfSample::accl() const {
  JgpmfVec3f const* p = nullptr;
  size_t n = 0u;
  if (!jgpmf_sample_accl(impl_->handle, &p, &n) || n == 0u) return {};
  // JgpmfVec3f is layout-compatible with Vec3f (three contiguous floats).
  // Reinterpret rather than copy.
  static_assert(sizeof(JgpmfVec3f) == sizeof(Vec3f));
  return { reinterpret_cast<Vec3f const*>(p), n };
}

std::span<Vec3f const> GpmfSample::gyro() const {
  JgpmfVec3f const* p = nullptr;
  size_t n = 0u;
  if (!jgpmf_sample_gyro(impl_->handle, &p, &n) || n == 0u) return {};
  return { reinterpret_cast<Vec3f const*>(p), n };
}

std::span<Vec3f const> GpmfSample::grav() const {
  JgpmfVec3f const* p = nullptr;
  size_t n = 0u;
  if (!jgpmf_sample_grav(impl_->handle, &p, &n) || n == 0u) return {};
  return { reinterpret_cast<Vec3f const*>(p), n };
}

std::span<Quatf const> GpmfSample::cori() const {
  JgpmfQuatf const* p = nullptr;
  size_t n = 0u;
  if (!jgpmf_sample_cori(impl_->handle, &p, &n) || n == 0u) return {};
  static_assert(sizeof(JgpmfQuatf) == sizeof(Quatf));
  return { reinterpret_cast<Quatf const*>(p), n };
}

std::span<Quatf const> GpmfSample::iori() const {
  JgpmfQuatf const* p = nullptr;
  size_t n = 0u;
  if (!jgpmf_sample_iori(impl_->handle, &p, &n) || n == 0u) return {};
  return { reinterpret_cast<Quatf const*>(p), n };
}

} // namespace mgpmf
