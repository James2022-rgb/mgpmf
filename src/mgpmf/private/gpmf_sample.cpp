#include "mgpmf/public/gpmf_sample.h"

// c++ system headers -----------------------------------
#include <cstring>

// external headers -------------------------------------
#include "jgpmf_capi.h"

namespace mgpmf {

namespace {

template <typename CType, typename CppType>
std::span<CppType const> BorrowSpan(JgpmfStatus status, CType const* p, size_t n) {
  if (status != JGPMF_OK || p == nullptr || n == 0u) return {};
  static_assert(sizeof(CType) == sizeof(CppType),
                "C and C++ layout must match for reinterpret cast.");
  return { reinterpret_cast<CppType const*>(p), n };
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

  JgpmfSample* h = nullptr;
  JgpmfStatus const rc = jgpmf_sample_parse(bytes, static_cast<size_t>(len), &h);
  if (rc != JGPMF_OK || h == nullptr) return nullptr;

  auto sample = std::unique_ptr<GpmfSample>(new GpmfSample());
  sample->impl_->handle = h;
  return sample;
}

GpmfSample::GpmfSample() : impl_(std::make_unique<Impl>()) {}
GpmfSample::~GpmfSample() = default;

std::optional<Gps9> GpmfSample::gps9() const {
  JgpmfGps9 raw{};
  if (jgpmf_sample_get_gps9(impl_->handle, &raw) != JGPMF_OK) return std::nullopt;

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
  JgpmfVec3 const* p = nullptr;
  size_t n = 0u;
  JgpmfStatus const rc = jgpmf_sample_accl(impl_->handle, &p, &n);
  return BorrowSpan<JgpmfVec3, Vec3f>(rc, p, n);
}

std::span<Vec3f const> GpmfSample::gyro() const {
  JgpmfVec3 const* p = nullptr;
  size_t n = 0u;
  JgpmfStatus const rc = jgpmf_sample_gyro(impl_->handle, &p, &n);
  return BorrowSpan<JgpmfVec3, Vec3f>(rc, p, n);
}

std::span<Vec3f const> GpmfSample::grav() const {
  JgpmfVec3 const* p = nullptr;
  size_t n = 0u;
  JgpmfStatus const rc = jgpmf_sample_grav(impl_->handle, &p, &n);
  return BorrowSpan<JgpmfVec3, Vec3f>(rc, p, n);
}

std::span<Quatf const> GpmfSample::cori() const {
  JgpmfQuat const* p = nullptr;
  size_t n = 0u;
  JgpmfStatus const rc = jgpmf_sample_cori(impl_->handle, &p, &n);
  return BorrowSpan<JgpmfQuat, Quatf>(rc, p, n);
}

std::span<Quatf const> GpmfSample::iori() const {
  JgpmfQuat const* p = nullptr;
  size_t n = 0u;
  JgpmfStatus const rc = jgpmf_sample_iori(impl_->handle, &p, &n);
  return BorrowSpan<JgpmfQuat, Quatf>(rc, p, n);
}

} // namespace mgpmf
