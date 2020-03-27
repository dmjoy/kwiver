#include <pybind11/pybind11.h>

#include <python/kwiver/arrows/core/transfer_with_depth_map.h>

namespace py = pybind11;

PYBIND11_MODULE(core, m)
{
  transfer_with_depth_map(m);
}
