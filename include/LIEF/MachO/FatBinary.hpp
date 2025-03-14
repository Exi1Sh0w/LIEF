/* Copyright 2017 - 2021 R. Thomas
 * Copyright 2017 - 2021 Quarkslab
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef LIEF_MACHO_FAT_BINARY_H_
#define LIEF_MACHO_FAT_BINARY_H_
#include <string>
#include <vector>
#include <memory>

#include "LIEF/types.hpp"
#include "LIEF/visibility.h"

#include "LIEF/MachO/enums.hpp"
#include "LIEF/MachO/type_traits.hpp"

namespace LIEF {
class Parser;
namespace MachO {

class Parser;
class Builder;
class Binary;

class LIEF_API FatBinary {

  friend class LIEF::Parser;
  friend class Parser;
  friend class Builder;

  public:
  FatBinary(const FatBinary&) = delete;
  FatBinary& operator=(const FatBinary&) = delete;

  virtual ~FatBinary(void);

  //! @brief Number of @link MachO::Binary binary @endlink registred
  size_t size(void) const;

  it_binaries begin(void);
  it_const_binaries begin(void) const;

  it_binaries end(void);
  it_const_binaries end(void) const;

  Binary* pop_back(void);

  Binary&       at(size_t index);
  const Binary& at(size_t index) const;

  Binary&       back(void);
  const Binary& back(void) const;

  Binary&       operator[](size_t index);
  const Binary& operator[](size_t index) const;

  //! Extract a MachO::Binary object. Gives ownership to the caller, and
  //! remove it from this FatBinary object.
  //!
  //! @warning: this invalidates any previously hold iterator!
  std::unique_ptr<Binary> take(size_t index);

  //! Take the underlying MachO::Binary that matches the given architecture
  //! If no binary with the architecture can be found, return a nullptr
  std::unique_ptr<Binary> take(CPU_TYPES cpu);

  //! Reconstruct the Fat binary object and write it in `filename`
  //! @param filename Path to write the reconstructed binary
  void write(const std::string& filename);

  //! Reconstruct the Fat binary object and return his content as bytes
  std::vector<uint8_t> raw(void);

  LIEF_API friend std::ostream& operator<<(std::ostream& os, const FatBinary& fatbinary);

  private:
  FatBinary(void);
  FatBinary(const std::vector<Binary*>& binaries);
  binaries_t binaries_;
};

} // namespace MachO
} // namespace LIEF
#endif
