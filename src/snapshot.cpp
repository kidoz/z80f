#include <z80f/snapshot.hpp>

// Snapshot is value-only; it lives entirely in the header. This translation
// unit exists to keep snapshot listed in the library sources for the public
// install_headers manifest and to anchor any future serialization helpers.

namespace z80f {

[[maybe_unused]] static inline void snapshot_anchor() {}

}  // namespace z80f
