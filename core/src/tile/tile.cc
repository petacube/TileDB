/**
 * @file   tile.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 TileDB, Inc.
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
 * This file implements class Tile.
 */

#include "tile.h"
#include "logger.h"

#include <iostream>

namespace tiledb {

/* ****************************** */
/*   CONSTRUCTORS & DESTRUCTORS   */
/* ****************************** */

Tile::Tile() {
  buffer_ = nullptr;
  cell_size_ = 0;
  compressor_ = Compressor::NO_COMPRESSION;
  compression_level_ = -1;
  file_offset_ = 0; // TODO: perhaps initialize to -1
  stores_offsets_ = false;
  tile_size_ = 0;
  type_ = Datatype::INT32;
}

Tile::Tile(
    Datatype type,
    Compressor compressor,
    int compression_level,
    uint64_t tile_size,
    uint64_t cell_size,
    bool stores_offsets)
    : cell_size_(cell_size)
    , compressor_(compressor)
    , compression_level_(compression_level)
    , stores_offsets_(stores_offsets)
    , tile_size_(tile_size)
    , type_(type) {
  buffer_ = nullptr;
  file_offset_ = 0; // TODO: perhaps initialize to -1
}

Tile::~Tile() {
  if (buffer_ != nullptr)
    delete buffer_;
}

/* ****************************** */
/*               API              */
/* ****************************** */

Status Tile::mmap(int fd, uint64_t tile_size, uint64_t offset) {
  // Create new buffer
  if (buffer_ != nullptr)
    delete buffer_;
  buffer_ = new Buffer();

  return buffer_->mmap(fd, tile_size, offset, !stores_offsets_);
}


Status Tile::write(ConstBuffer* const_buffer) {
  if (buffer_ == nullptr)
    buffer_ = new Buffer(tile_size_);

  if (buffer_->size() == 0)
    LOG_STATUS(
        Status::TileError("Cannot write into tile; Buffer allocation failed"));

  buffer_->write(const_buffer);

  return Status::Ok();
}

Status Tile::write(ConstBuffer* const_buffer, uint64_t bytes) {
  if (buffer_ == nullptr)
    buffer_ = new Buffer(tile_size_);

  if (buffer_->size() == 0)
    LOG_STATUS(
        Status::TileError("Cannot write into tile; Buffer allocation failed"));

  buffer_->write(const_buffer, bytes);

  return Status::Ok();
}

/* ****************************** */
/*          PRIVATE METHODS       */
/* ****************************** */

}  // namespace tiledb