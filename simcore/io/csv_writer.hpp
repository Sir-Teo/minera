#pragma once
#include "simcore/io/writer.hpp"
#include <string>

namespace minerva {

struct CSVWriterConfig {
  std::string output_dir{"output"};
  std::string prefix{"sim"};
  bool write_rigid_bodies{true};
  bool write_md_particles{true};
};

class CSVWriter final : public IWriter {
public:
  explicit CSVWriter(const CSVWriterConfig& cfg);
  ~CSVWriter() override = default;

  void write(const World& world, int frame_number) override;

private:
  CSVWriterConfig cfg_;
  bool initialized_{false};

  void ensure_output_dir();
  void write_rigid_bodies(const World& world, int frame_number);
  void write_md_particles(const World& world, int frame_number);
};

} // namespace minerva
