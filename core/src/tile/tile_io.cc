/**
 * @file   tile_io.cc
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
 * This file implements class TileIO.
 */

#include "tile_io.h"
#include "blosc_compressor.h"
#include "bzip_compressor.h"
#include "filesystem.h"
#include "gzip_compressor.h"
#include "logger.h"
#include "lz4_compressor.h"
#include "rle_compressor.h"
#include "utils.h"
#include "zstd_compressor.h"

#include <iostream>

namespace tiledb {

/* ****************************** */
/*   CONSTRUCTORS & DESTRUCTORS   */
/* ****************************** */

TileIO::TileIO(const Configurator* config, const uri::URI& attr_filename)
    : attr_filename_(attr_filename)
    , config_(config) {
  buffer_ = nullptr;
}

TileIO::~TileIO() {
  if (buffer_ != nullptr)
    delete buffer_;
}

/* ****************************** */
/*               API              */
/* ****************************** */

Status TileIO::read(
    Tile* tile,
    uint64_t file_offset,
    uint64_t compressed_size,
    uint64_t tile_size) {
  // For easy reference
  Compressor compression = tile->compressor();
  IOMethod read_method = config_->read_method();

  if (compression == Compressor::NO_COMPRESSION) {
    if (read_method == IOMethod::READ || read_method == IOMethod::MPI)
      tile->set_file_offset(file_offset);
    else if (read_method == IOMethod::MMAP)
      RETURN_NOT_OK(map_tile(tile, compressed_size, file_offset));
  } else {
    if (buffer_ == nullptr)
      buffer_ = new Buffer(compressed_size);
    if (read_method == IOMethod::READ) {
      RETURN_NOT_OK(filesystem::read_from_file(
          attr_filename_.to_string(),
          file_offset,
          buffer_->data(),
          compressed_size));
    } else if (read_method == IOMethod::MMAP) {
      RETURN_NOT_OK(map_tile(compressed_size, file_offset));
    } else if (read_method == IOMethod::MPI) {
#ifdef HAVE_MPI
      RETURN_NOT_OK(filesystem::mpi_io_read_from_file(
          config_->mpi_comm(),
          attr_filename_.to_string(),
          file_offset,
          buffer_->data(),
          compressed_size));
#else
      // Error: MPI not supported
      return LOG_STATUS(
          Status::TileIOError("Cannot read tile; MPI not supported"));
#endif
    }
  }

  // Decompress tile
  // TODO: here we will put all other filters, and potentially employ chunking
  // TODO: choose the proper buffer based on all filters, not just compression
  RETURN_NOT_OK(decompress_tile(tile, tile_size));
}

Status TileIO::write(Tile* tile, uint64_t* bytes_written) {
  // Compress tile
  // TODO: here we will put all other filters, and potentially employ chunking
  // TODO: choose the proper buffer based on all filters, not just compression
  RETURN_NOT_OK(compress_tile(tile));

  // Prepare to write
  IOMethod write_method = config_->write_method();
  Compressor compressor = tile->compressor();
  void* buffer = (compressor == Compressor::NO_COMPRESSION) ? tile->data() :
                                                              buffer_->data();
  uint64_t buffer_size = (compressor == Compressor::NO_COMPRESSION) ?
                             tile->offset() :
                             buffer_->offset();
  *bytes_written = buffer_size;

  // Write based on the chosen method
  // TODO: implement mmap write here
  if (write_method == IOMethod::WRITE) {
    return filesystem::write_to_file(
        attr_filename_.to_posix_path(), buffer, buffer_size);
  } else if (write_method == IOMethod::MPI) {
#ifdef HAVE_MPI
    return filesystem::mpi_io_write_to_file(
        config_->mpi_comm(),
        attr_filename_.to_posix_path().c_str(),
        buffer,
        buffer_size);
#else
    // Error: MPI not supported
    return LOG_STATUS(
        Status::TileIOError("Cannot write tile; MPI not supported"));
#endif
  } else {
    return LOG_STATUS(
        Status::TileIOError("Cannot write tile; Unknown write method"));
  }
}

/* ****************************** */
/*          PRIVATE METHODS       */
/* ****************************** */

Status TileIO::compress_tile(Tile* tile) {
  // For easy reference
  Compressor compression = tile->compressor();
  int level = tile->compression_level();

  // Reset buffer
  if (buffer_ != nullptr)
    buffer_->reset();

  // Handle different compression
  switch (compression) {
    case Compressor::NO_COMPRESSION:
      return Status::Ok();
    case Compressor::GZIP:
      return compress_tile_gzip(tile, level);
    case Compressor::ZSTD:
      return compress_tile_zstd(tile, level);
    case Compressor::LZ4:
      return compress_tile_lz4(tile, level);
    case Compressor::BLOSC:
      return compress_tile_blosc(tile, level, "blosclz");
#undef BLOSC_LZ4
    case Compressor::BLOSC_LZ4:
      return compress_tile_blosc(tile, level, "lz4");
#undef BLOSC_LZ4HC
    case Compressor::BLOSC_LZ4HC:
      return compress_tile_blosc(tile, level, "lz4hc");
#undef BLOSC_SNAPPY
    case Compressor::BLOSC_SNAPPY:
      return compress_tile_blosc(tile, level, "snappy");
#undef BLOSC_ZLIB
    case Compressor::BLOSC_ZLIB:
      return compress_tile_blosc(tile, level, "zlib");
#undef BLOSC_ZSTD
    case Compressor::BLOSC_ZSTD:
      return compress_tile_blosc(tile, level, "zstd");
    case Compressor::RLE:
      return compress_tile_rle(tile, level);
    case Compressor::BZIP2:
      return compress_tile_bzip2(tile, level);
  }
}

Status TileIO::compress_tile_gzip(Tile* tile, int level) {
  // Allocate space to store the compressed tile
  uint64_t tile_size = tile->offset();
  uint64_t gzip_overhead = GZip::overhead(tile_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(tile_size + gzip_overhead);
  while (tile_size + gzip_overhead > buffer_->size_alloced())
    buffer_->realloc(tile_size + gzip_overhead);

  // Compress tile
  size_t buffer_size;  // TODO: remove this, changing the compress signature
  // TODO: These functions should take as input buffers instead
  RETURN_NOT_OK(GZip::compress(
      utils::datatype_size(tile->type()),
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &buffer_size));
  buffer_->set_size(buffer_size);    // TODO: this will change
  buffer_->set_offset(buffer_size);  // TODO: this will change

  return Status::Ok();
}

Status TileIO::compress_tile_zstd(Tile* tile, int level) {
  // Allocate space to store the compressed tile
  uint64_t tile_size = tile->offset();
  size_t compress_bound = ZStd::compress_bound(tile_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(compress_bound);
  while (compress_bound > buffer_->size_alloced())
    buffer_->realloc(compress_bound);

  size_t buffer_size;  // TODO: remove this, changing the compress signature
  RETURN_NOT_OK(ZStd::compress(
      utils::datatype_size(tile->type()),
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &buffer_size));
  buffer_->set_size(buffer_size);
  buffer_->set_offset(buffer_size);  // TODO: this will change

  return Status::Ok();
}

Status TileIO::compress_tile_lz4(Tile* tile, int level) {
  // Allocate space to store the compressed tile
  uint64_t tile_size = tile->offset();
  size_t compress_bound = LZ4::compress_bound(tile_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(compress_bound);
  while (compress_bound > buffer_->size_alloced())
    buffer_->realloc(compress_bound);

  // Compress tile
  size_t buffer_size;  // TODO: remove this, changing the compress signature
  RETURN_NOT_OK(LZ4::compress(
      utils::datatype_size(tile->type()),
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &buffer_size));
  buffer_->set_size(buffer_size);
  buffer_->set_offset(buffer_size);  // TODO: this will change

  // Success
  return Status::Ok();
}

Status TileIO::compress_tile_blosc(
    Tile* tile, int level, const char* compressor) {
  // Allocate space to store the compressed tile
  uint64_t tile_size = tile->offset();
  size_t compress_bound = Blosc::compress_bound(tile_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(compress_bound);
  while (compress_bound > buffer_->size_alloced())
    buffer_->realloc(compress_bound);

  // Compress tile
  size_t buffer_size;  // TODO: remove this, changing the compress signature
  RETURN_NOT_OK(Blosc::compress(
      compressor,
      utils::datatype_size(tile->type()),
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &buffer_size));
  buffer_->set_size(buffer_size);
  buffer_->set_offset(buffer_size);  // TODO: this will change

  return Status::Ok();
}

Status TileIO::compress_tile_rle(Tile* tile, int level) {
  uint64_t tile_size = tile->offset();
  uint64_t value_size = tile->cell_size();
  size_t compress_bound = RLE::compress_bound(tile_size, value_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(compress_bound);
  while (compress_bound > buffer_->size_alloced())
    buffer_->realloc(compress_bound);

  // Compress tile
  size_t rle_size = 0;
  RETURN_NOT_OK(RLE::compress(
      value_size,
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &rle_size));
  buffer_->set_size(rle_size);
  buffer_->set_offset(rle_size);  // TODO: this will change

  // Success
  return Status::Ok();
}

Status TileIO::compress_tile_bzip2(Tile* tile, int level) {
  // Allocate space to store the compressed tile
  uint64_t tile_size = tile->offset();
  size_t compress_bound = BZip::compress_bound(tile_size);
  if (buffer_ == nullptr)
    buffer_ = new Buffer(compress_bound);
  while (compress_bound > buffer_->size_alloced())
    buffer_->realloc(compress_bound);

  // Compress tile
  size_t buffer_size;  // TODO: remove this, changing the compress signature
  RETURN_NOT_OK(BZip::compress(
      utils::datatype_size(tile->type()),
      level,
      tile->data(),
      tile_size,
      buffer_->data(),
      buffer_->size_alloced(),
      &buffer_size));
  buffer_->set_size(buffer_size);
  buffer_->set_offset(buffer_size);  // TODO: this will change

  // Success
  return Status::Ok();
}

Status TileIO::decompress_tile(Tile* tile, uint64_t tile_size {
  // For easy reference
  Compressor compression = tile->compressor();

  switch (compression) {
    case Compressor::GZIP:
      return decompress_tile_gzip(
          attribute_id, tile_compressed, tile_compressed_size, tile, tile_size);
    case Compressor::ZSTD:
      return decompress_tile_zstd(
          attribute_id, tile_compressed, tile_compressed_size, tile, tile_size);
    case Compressor::LZ4:
      return decompress_tile_lz4(
          attribute_id, tile_compressed, tile_compressed_size, tile, tile_size);
    case Compressor::BLOSC:
#undef BLOSC_LZ4
    case Compressor::BLOSC_LZ4:
#undef BLOSC_LZ4HC
    case Compressor::BLOSC_LZ4HC:
#undef BLOSC_SNAPPY
    case Compressor::BLOSC_SNAPPY:
#undef BLOSC_ZLIB
    case Compressor::BLOSC_ZLIB:
#undef BLOSC_ZSTD
    case Compressor::BLOSC_ZSTD:
      return decompress_tile_blosc(
          attribute_id, tile_compressed, tile_compressed_size, tile, tile_size);
    case Compressor::RLE:
      return decompress_tile_rle(
          attribute_id,
          cell_size,
          tile_compressed,
          tile_compressed_size,
          tile,
          tile_size);
    case Compressor::BZIP2:
      return decompress_tile_bzip2(
          attribute_id, tile_compressed, tile_compressed_size, tile, tile_size);
    case Compressor::NO_COMPRESSION:
      return Status::Ok();
  }
}

 Status ReadState::decompress_tile_gzip(
    int attribute_id,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // For easy reference
  const ArraySchema* array_schema = fragment_->array()->array_schema();
  int dim_num = array_schema->dim_num();
  int attribute_num = array_schema->attribute_num();
  bool coords = (attribute_id == attribute_num);
  size_t coords_size = array_schema->coords_size();

  // Decompress tile
  size_t out_size = 0;
  RETURN_NOT_OK(GZip::decompress(
      array_schema->type_size(attribute_id),
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size));

  return Status::Ok();
}

Status ReadState::decompress_tile_zstd(
    int attribute_id,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // For easy reference
  const ArraySchema* array_schema = fragment_->array()->array_schema();
  int dim_num = array_schema->dim_num();
  int attribute_num = array_schema->attribute_num();
  bool coords = (attribute_id == attribute_num);
  size_t coords_size = array_schema->coords_size();

  // Decompress tile
  size_t out_size = 0;
  RETURN_NOT_OK(ZStd::decompress(
      array_schema->type_size(attribute_id),
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size));

  // Success
  return Status::Ok();
}

Status ReadState::decompress_tile_lz4(
    int attribute_id,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // For easy reference
  const ArraySchema* array_schema = fragment_->array()->array_schema();
  int dim_num = array_schema->dim_num();
  int attribute_num = array_schema->attribute_num();
  bool coords = (attribute_id == attribute_num);
  size_t coords_size = array_schema->coords_size();

  size_t out_size = 0;
  RETURN_NOT_OK(LZ4::decompress(
      array_schema->type_size(attribute_id),
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size));

  // Success
  return Status::Ok();
}

Status ReadState::decompress_tile_blosc(
    int attribute_id,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // For easy reference
  const ArraySchema* array_schema = fragment_->array()->array_schema();
  int dim_num = array_schema->dim_num();
  int attribute_num = array_schema->attribute_num();
  bool coords = (attribute_id == attribute_num);
  size_t coords_size = array_schema->coords_size();

  size_t out_size = 0;
  RETURN_NOT_OK(Blosc::decompress(
      array_schema->type_size(attribute_id),
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size));

  // Success
  return Status::Ok();
}

Status ReadState::decompress_tile_rle(
    int attribute_id,
    uint64_t value_size,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // Decompress tile
  Status st;
  size_t out_size;
  st = RLE::decompress(
      value_size,
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size);

  return st;
}

Status ReadState::decompress_tile_bzip2(
    int attribute_id,
    unsigned char* tile_compressed,
    size_t tile_compressed_size,
    unsigned char* tile,
    size_t tile_size) {
  // For easy reference
  const ArraySchema* array_schema = fragment_->array()->array_schema();
  int dim_num = array_schema->dim_num();
  int attribute_num = array_schema->attribute_num();
  bool coords = (attribute_id == attribute_num);
  size_t coords_size = array_schema->coords_size();

  // Decompress tile
  size_t out_size = 0;
  RETURN_NOT_OK(BZip::decompress(
      array_schema->type_size(attribute_id),
      tile_compressed,
      tile_compressed_size,
      tile,
      tile_size,
      &out_size));

  return Status::Ok();
}

 Status TileIO::map_tile(Tile* tile, uint64_t tile_size, uint64_t offset) {
  // TODO: this probably will not work with anything non-POSIX
  // Open file
  int fd = open(attr_filename_.c_str(), O_RDONLY);
  if(fd == -1)
    return LOG_STATUS(
            Status::TileIOError("Cannot map tile; File opening error"));

  RETURN_NOT_OK_ELSE(tile->mmap(fd, tile_size, offset), close(fd));

  // Close file
  if (close(fd))
    return LOG_STATUS(
            Status::TileIOError("Cannot map tile; File closing error"));

  // Success
  return Status::Ok();
}

 Status TileIO::map_tile(uint64_t tile_size, uint64_t offset) {
  // TODO: this probably will not work with anything non-POSIX
  // Unmap
  if (map_addr_compressed_ != nullptr) {
    if (munmap(map_addr_compressed_, map_addr_compressed_length_)) {
      return LOG_STATUS(Status::MMapError(
          "Cannot read tile from file with map; Memory unmap error"));
    }
  }

  // Calculate offset considering the page size
  size_t page_size = sysconf(_SC_PAGE_SIZE);
  off_t start_offset = (offset / page_size) * page_size;
  size_t extra_offset = offset - start_offset;
  size_t new_length = tile_size + extra_offset;

  // Open file
  int fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    munmap(map_addr_compressed_, map_addr_compressed_length_);
    map_addr_compressed_ = nullptr;
    map_addr_compressed_length_ = 0;
    tile_compressed_ = nullptr;
    return LOG_STATUS(
        Status::MMapError("Cannot read tile from file; File opening error"));
  }

  // Map
  map_addr_compressed_ = mmap(
      map_addr_compressed_,
      new_length,
      PROT_READ,
      MAP_SHARED,
      fd,
      start_offset);
  if (map_addr_compressed_ == MAP_FAILED) {
    map_addr_compressed_ = nullptr;
    map_addr_compressed_length_ = 0;
    tile_compressed_ = nullptr;
    return LOG_STATUS(
        Status::MMapError("Cannot read tile from file; Memory map error"));
  }
  map_addr_compressed_length_ = new_length;

  // Set properly the compressed tile pointer
  tile_compressed_ = static_cast<char*>(map_addr_compressed_) + extra_offset;

  // Close file
  if (close(fd)) {
    munmap(map_addr_compressed_, map_addr_compressed_length_);
    map_addr_compressed_ = nullptr;
    map_addr_compressed_length_ = 0;
    tile_compressed_ = nullptr;
    return LOG_STATUS(
        Status::OSError("Cannot read tile from file; File closing error"));
  }

  return Status::Ok();
}

}  // namespace tiledb
