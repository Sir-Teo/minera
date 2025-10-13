#pragma once
#include "simcore/io/writer.hpp"
#include <string>
#include <vector>

namespace minerva {

struct VTKWriterConfig {
  std::string output_dir{"output"};
  std::string prefix{"sim"};
  bool write_rigid_bodies{true};
  bool write_md_particles{true};
};

class VTKWriter final : public IWriter {
public:
  explicit VTKWriter(const VTKWriterConfig& cfg);
  ~VTKWriter() override = default;

  void write(const World& world, int frame_number) override;
  void finalize() override;

private:
  VTKWriterConfig cfg_;
  bool initialized_{false};

  void ensure_output_dir();
  void write_rigid_bodies_vtu(const World& world, int frame_number);
  void write_md_particles_vtu(const World& world, int frame_number);
  void write_pvd_collection();

  // Track frame numbers for PVD collection file
  std::vector<int> rb_frames_;
  std::vector<int> md_frames_;
};

} // namespace minerva
