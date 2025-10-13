#include "simcore/io/vtk_writer.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <cstring>

namespace minerva {

VTKWriter::VTKWriter(const VTKWriterConfig& cfg) : cfg_(cfg) {}

void VTKWriter::ensure_output_dir() {
  if (!initialized_) {
    mkdir(cfg_.output_dir.c_str(), 0755);
    initialized_ = true;
    MINERVA_LOG("VTK output directory: %s\n", cfg_.output_dir.c_str());
  }
}

void VTKWriter::write(const World& world, int frame_number) {
  ensure_output_dir();

  if (cfg_.write_rigid_bodies && !world.rigid_bodies.empty()) {
    write_rigid_bodies_vtu(world, frame_number);
    rb_frames_.push_back(frame_number);
  }

  if (cfg_.write_md_particles && world.md_particles.size() > 0) {
    write_md_particles_vtu(world, frame_number);
    md_frames_.push_back(frame_number);
  }
}

void VTKWriter::write_rigid_bodies_vtu(const World& world, int frame_number) {
  std::ostringstream filename;
  filename << cfg_.output_dir << "/" << cfg_.prefix << "_rb_"
           << std::setfill('0') << std::setw(6) << frame_number << ".vtu";

  std::ofstream out(filename.str());
  if (!out) {
    MINERVA_LOG("Warning: Could not open %s\n", filename.str().c_str());
    return;
  }

  const std::size_t n = world.rigid_bodies.size();

  // Write VTK XML header
  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
  out << "  <UnstructuredGrid>\n";
  out << "    <Piece NumberOfPoints=\"" << n << "\" NumberOfCells=\"" << n << "\">\n";

  // Points (positions)
  out << "      <Points>\n";
  out << "        <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n";
  for (const auto& rb : world.rigid_bodies) {
    out << "          " << rb.position.x << " " << rb.position.y << " " << rb.position.z << "\n";
  }
  out << "        </DataArray>\n";
  out << "      </Points>\n";

  // Cells (one vertex per particle)
  out << "      <Cells>\n";
  out << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "          " << i << "\n";
  }
  out << "        </DataArray>\n";
  out << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n";
  for (std::size_t i = 1; i <= n; ++i) {
    out << "          " << i << "\n";
  }
  out << "        </DataArray>\n";
  out << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "          1\n"; // VTK_VERTEX = 1
  }
  out << "        </DataArray>\n";
  out << "      </Cells>\n";

  // Point data (velocities, mass, radius)
  out << "      <PointData Vectors=\"velocity\" Scalars=\"mass\">\n";

  // Velocity
  out << "        <DataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\" format=\"ascii\">\n";
  for (const auto& rb : world.rigid_bodies) {
    out << "          " << rb.velocity.x << " " << rb.velocity.y << " " << rb.velocity.z << "\n";
  }
  out << "        </DataArray>\n";

  // Mass
  out << "        <DataArray type=\"Float32\" Name=\"mass\" format=\"ascii\">\n";
  for (const auto& rb : world.rigid_bodies) {
    out << "          " << rb.mass << "\n";
  }
  out << "        </DataArray>\n";

  // Radius
  out << "        <DataArray type=\"Float32\" Name=\"radius\" format=\"ascii\">\n";
  for (const auto& rb : world.rigid_bodies) {
    out << "          " << rb.radius << "\n";
  }
  out << "        </DataArray>\n";

  // Kinematic flag
  out << "        <DataArray type=\"Int32\" Name=\"kinematic\" format=\"ascii\">\n";
  for (const auto& rb : world.rigid_bodies) {
    out << "          " << (rb.kinematic ? 1 : 0) << "\n";
  }
  out << "        </DataArray>\n";

  out << "      </PointData>\n";
  out << "    </Piece>\n";
  out << "  </UnstructuredGrid>\n";
  out << "</VTKFile>\n";

  out.close();
}

void VTKWriter::write_md_particles_vtu(const World& world, int frame_number) {
  std::ostringstream filename;
  filename << cfg_.output_dir << "/" << cfg_.prefix << "_md_"
           << std::setfill('0') << std::setw(6) << frame_number << ".vtu";

  std::ofstream out(filename.str());
  if (!out) {
    MINERVA_LOG("Warning: Could not open %s\n", filename.str().c_str());
    return;
  }

  const std::size_t n = world.md_particles.size();

  // Write VTK XML header
  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
  out << "  <UnstructuredGrid>\n";
  out << "    <Piece NumberOfPoints=\"" << n << "\" NumberOfCells=\"" << n << "\">\n";

  // Points
  out << "      <Points>\n";
  out << "        <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    const auto& p = world.md_particles[i];
    out << "          " << p.position.x << " " << p.position.y << " " << p.position.z << "\n";
  }
  out << "        </DataArray>\n";
  out << "      </Points>\n";

  // Cells
  out << "      <Cells>\n";
  out << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "          " << i << "\n";
  }
  out << "        </DataArray>\n";
  out << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n";
  for (std::size_t i = 1; i <= n; ++i) {
    out << "          " << i << "\n";
  }
  out << "        </DataArray>\n";
  out << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "          1\n"; // VTK_VERTEX
  }
  out << "        </DataArray>\n";
  out << "      </Cells>\n";

  // Point data
  out << "      <PointData Vectors=\"velocity\" Scalars=\"mass\">\n";

  // Velocity
  out << "        <DataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    const auto& p = world.md_particles[i];
    out << "          " << p.velocity.x << " " << p.velocity.y << " " << p.velocity.z << "\n";
  }
  out << "        </DataArray>\n";

  // Mass
  out << "        <DataArray type=\"Float32\" Name=\"mass\" format=\"ascii\">\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "          " << world.md_particles[i].mass << "\n";
  }
  out << "        </DataArray>\n";

  out << "      </PointData>\n";
  out << "    </Piece>\n";
  out << "  </UnstructuredGrid>\n";
  out << "</VTKFile>\n";

  out.close();
}

void VTKWriter::finalize() {
  if (!initialized_) return;

  write_pvd_collection();
  MINERVA_LOG("VTK output finalized. Open .pvd files in ParaView.\n");
}

void VTKWriter::write_pvd_collection() {
  // Write rigid bodies collection
  if (!rb_frames_.empty()) {
    std::ostringstream filename;
    filename << cfg_.output_dir << "/" << cfg_.prefix << "_rb.pvd";

    std::ofstream out(filename.str());
    if (out) {
      out << "<?xml version=\"1.0\"?>\n";
      out << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
      out << "  <Collection>\n";

      for (int frame : rb_frames_) {
        std::ostringstream vtu_name;
        vtu_name << cfg_.prefix << "_rb_" << std::setfill('0') << std::setw(6) << frame << ".vtu";
        out << "    <DataSet timestep=\"" << frame << "\" file=\"" << vtu_name.str() << "\"/>\n";
      }

      out << "  </Collection>\n";
      out << "</VTKFile>\n";
      out.close();
    }
  }

  // Write MD particles collection
  if (!md_frames_.empty()) {
    std::ostringstream filename;
    filename << cfg_.output_dir << "/" << cfg_.prefix << "_md.pvd";

    std::ofstream out(filename.str());
    if (out) {
      out << "<?xml version=\"1.0\"?>\n";
      out << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
      out << "  <Collection>\n";

      for (int frame : md_frames_) {
        std::ostringstream vtu_name;
        vtu_name << cfg_.prefix << "_md_" << std::setfill('0') << std::setw(6) << frame << ".vtu";
        out << "    <DataSet timestep=\"" << frame << "\" file=\"" << vtu_name.str() << "\"/>\n";
      }

      out << "  </Collection>\n";
      out << "</VTKFile>\n";
      out.close();
    }
  }
}

} // namespace minerva
