#include "simcore/io/csv_writer.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

namespace minerva {

CSVWriter::CSVWriter(const CSVWriterConfig& cfg) : cfg_(cfg) {}

void CSVWriter::ensure_output_dir() {
  if (!initialized_) {
    // Create output directory (simple POSIX approach)
    mkdir(cfg_.output_dir.c_str(), 0755);
    initialized_ = true;
    MINERVA_LOG("CSV output directory: %s\n", cfg_.output_dir.c_str());
  }
}

void CSVWriter::write(const World& world, int frame_number) {
  ensure_output_dir();

  if (cfg_.write_rigid_bodies) {
    write_rigid_bodies(world, frame_number);
  }

  if (cfg_.write_md_particles) {
    write_md_particles(world, frame_number);
  }
}

void CSVWriter::write_rigid_bodies(const World& world, int frame_number) {
  std::ostringstream filename;
  filename << cfg_.output_dir << "/" << cfg_.prefix << "_rb_"
           << std::setfill('0') << std::setw(6) << frame_number << ".csv";

  std::ofstream out(filename.str());
  if (!out) {
    MINERVA_LOG("Warning: Could not open %s\n", filename.str().c_str());
    return;
  }

  // Write header
  out << "id,x,y,z,vx,vy,vz,mass,radius,kinematic\n";

  // Write data
  for (std::size_t i = 0; i < world.rigid_bodies.size(); ++i) {
    const auto& rb = world.rigid_bodies[i];
    out << i << ","
        << rb.position.x << "," << rb.position.y << "," << rb.position.z << ","
        << rb.velocity.x << "," << rb.velocity.y << "," << rb.velocity.z << ","
        << rb.mass << "," << rb.radius << "," << (rb.kinematic ? 1 : 0) << "\n";
  }

  out.close();
}

void CSVWriter::write_md_particles(const World& world, int frame_number) {
  std::ostringstream filename;
  filename << cfg_.output_dir << "/" << cfg_.prefix << "_md_"
           << std::setfill('0') << std::setw(6) << frame_number << ".csv";

  std::ofstream out(filename.str());
  if (!out) {
    MINERVA_LOG("Warning: Could not open %s\n", filename.str().c_str());
    return;
  }

  // Write header
  out << "id,x,y,z,vx,vy,vz,mass\n";

  // Write data
  for (std::size_t i = 0; i < world.md_particles.size(); ++i) {
    const auto& p = world.md_particles[i];
    out << i << ","
        << p.position.x << "," << p.position.y << "," << p.position.z << ","
        << p.velocity.x << "," << p.velocity.y << "," << p.velocity.z << ","
        << p.mass << "\n";
  }

  out.close();
}

} // namespace minerva
