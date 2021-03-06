/**
 * @file   tiledb_cpp_api_dimension.h
 *
 * @author Ravi Gaddipati
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2018 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for the TileDB Dimension object.
 */

#ifndef TILEDB_CPP_API_DIMENSION_H
#define TILEDB_CPP_API_DIMENSION_H

#include "tiledb.h"
#include "tiledb_cpp_api_context.h"
#include "tiledb_cpp_api_deleter.h"
#include "tiledb_cpp_api_exception.h"
#include "tiledb_cpp_api_object.h"
#include "tiledb_cpp_api_type.h"

#include <functional>
#include <memory>

namespace tiledb {

/** Implements the dimension functionality. */
class Dimension {
 public:
  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  Dimension(const Context& ctx, tiledb_dimension_t* dim);
  Dimension(const Dimension&) = default;
  Dimension(Dimension&& o) = default;
  Dimension& operator=(const Dimension&) = default;
  Dimension& operator=(Dimension&& o) = default;

  /* ********************************* */
  /*                API                */
  /* ********************************* */

  /** Returns the name of the dimension. */
  const std::string name() const;

  /** Returns the dimension datatype. */
  tiledb_datatype_t type() const;

  /** Returns the domain of the dimension. **/
  template <typename T>
  std::pair<T, T> domain() const {
    static_assert(
        std::is_fundamental<T>::value,
        "Template type must be a fundamental type.");
    using DataT = typename impl::type_from_native<T>::type;

    auto tdbtype = type();
    if (DataT::tiledb_datatype != tdbtype) {
      throw TypeError::create<DataT>(tdbtype);
    }
    auto d = static_cast<T*>(_domain());
    return std::pair<T, T>(d[0], d[1]);
  };

  /** Returns a string representation of the domain. */
  std::string domain_to_str() const;

  /** Returns the tile extent of the dimension. */
  template <typename T>
  T tile_extent() const {
    static_assert(
        std::is_fundamental<T>::value,
        "Template type must be a fundamental type.");
    using DataT = typename impl::type_from_native<T>::type;

    auto tdbtype = type();
    if (DataT::tiledb_datatype != tdbtype) {
      throw TypeError::create<DataT>(tdbtype);
    }
    return *static_cast<T*>(_tile_extent());
  }

  /** Returns a string representation of the extent. */
  std::string tile_extent_to_str() const;

  /** Returns a shared pointer to the C TileDB dimension object. */
  std::shared_ptr<tiledb_dimension_t> ptr() const;

  /** Auxiliary operator for getting the underlying C TileDB object. */
  operator tiledb_dimension_t*() const;

  /* ********************************* */
  /*          STATIC FUNCTIONS         */
  /* ********************************* */

  /**
   * Factory function for creating a new dimension with datatype T.
   *
   * @tparam T int, char, etc...
   * @param ctx The TileDB context.
   * @param name The dimension name.
   * @param domain The dimension domain.
   * @return A new `Attribute` object.
   */
  template <typename T>
  static Dimension create(
      const Context& ctx,
      const std::string& name,
      const std::array<T, 2>& domain,
      T extent) {
    static_assert(
        std::is_fundamental<T>::value,
        "Template type must be a fundamental type.");
    return create_impl(
        ctx,
        name,
        impl::type_from_native<T>::type::tiledb_datatype,
        &domain,
        &extent);
  }

 private:
  /* ********************************* */
  /*         PRIVATE ATTRIBUTES        */
  /* ********************************* */

  /** The TileDB context. */
  std::reference_wrapper<const Context> ctx_;

  /** A deleter wrapper. */
  impl::Deleter deleter_;

  /** The C TileDB dimension object. */
  std::shared_ptr<tiledb_dimension_t> dim_;

  /* ********************************* */
  /*          PRIVATE METHODS          */
  /* ********************************* */

  /** Returns the binary representation of the dimension domain. */
  void* _domain() const;

  /** Returns the binary representation of the dimension extent. */
  void* _tile_extent() const;

  /* ********************************* */
  /*     PRIVATE STATIC FUNCTIONS      */
  /* ********************************* */

  /**
   * Creates a dimension with the input name, datatype, domain and tile
   * extent.
   */
  static Dimension create_impl(
      const Context& ctx,
      const std::string& name,
      tiledb_datatype_t type,
      const void* domain,
      const void* tile_extent);
};

/* ********************************* */
/*               MISC                */
/* ********************************* */

/** Get a string representation of a dimension for an output stream. */
std::ostream& operator<<(std::ostream& os, const Dimension& dim);

}  // namespace tiledb

#endif  // TILEDB_CPP_API_DIMENSION_H
