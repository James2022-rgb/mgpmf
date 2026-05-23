#pragma once

// c++ system headers -----------------------------------
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace mgpmf {

/// GPS9 fix snapshot. One value per `gpmd` MP4 sample at most;
/// `GpmfSample::gps9()` returns `nullopt` for samples without a fix.
/// Units mirror the GoPro convention -- latitude / longitude in
/// degrees, altitude in metres, speeds in m/s, DOP unitless, `fix`
/// the raw GoPro fix code.
struct Gps9 final {
  uint32_t fix                    = 0;
  float    dop                    = 0.0f;
  float    latitude               = 0.0f;
  float    longitude              = 0.0f;
  float    altitude               = 0.0f;
  float    speed_2d               = 0.0f;
  float    speed_3d               = 0.0f;
  /// UTC reconstruction: days since 2000-01-01 + seconds since UTC
  /// midnight. Plot these to a `chrono::system_clock::time_point` if
  /// you need wall-clock display.
  float    days_since_2000        = 0.0f;
  float    seconds_since_midnight = 0.0f;
};

struct Vec3f final {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Quatf final {
  float w = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

/// Owns one parsed `gpmd` MP4 sample (one chunk of GPMF bytes).
/// Accessor methods return spans aliasing into the sample's arena;
/// those spans become dangling when the `GpmfSample` is destroyed.
///
/// MP4 PTS / duration is not part of the sample -- the caller pulls
/// those from the MP4 demuxer and distributes them across the
/// returned spans (uniform inside the sample is the GoPro convention
/// for ~200 Hz ACCL, ~400 Hz GYRO, etc.).
class GpmfSample final {
public:
  /// Parses one `gpmd` MP4 sample's bytes. Returns `nullptr` on
  /// parse failure.
  static std::unique_ptr<GpmfSample> Parse(uint8_t const* bytes, uint32_t len);

  ~GpmfSample();
  GpmfSample(GpmfSample const&) = delete;
  GpmfSample& operator=(GpmfSample const&) = delete;

  /// `nullopt` when this sample carried no GPS9 fix.
  std::optional<Gps9> gps9() const;

  /// Accelerometer samples (m/s^2). Typically ~200 entries per
  /// MP4 sample.
  std::span<Vec3f const> accl() const;
  /// Gyroscope samples (rad/s). Typically ~400 entries per MP4
  /// sample.
  std::span<Vec3f const> gyro() const;
  /// Gravity vector samples (unit norm).
  std::span<Vec3f const> grav() const;
  /// Camera orientation quaternions, (w, x, y, z).
  std::span<Quatf const> cori() const;
  /// IMU orientation quaternions, (w, x, y, z).
  std::span<Quatf const> iori() const;

private:
  GpmfSample();

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace mgpmf
