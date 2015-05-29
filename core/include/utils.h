/**
 * @file   tile.h
 * @author Stavros Papadopoulos <stavrosp@csail.mit.edu>
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2014 Stavros Papadopoulos <stavrosp@csail.mit.edu>
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
 * This file contains useful (global) functions.
 */

#ifndef UTILS_H
#define UTILS_H

#include "array_schema.h"
#include "storage_manager.h"

/** Replaces '~' in the input path with the corresponding absolute path. */
std::string absolute_path(const std::string& path);

/** Converts the values of a into the data type of b and stores them in b. */
template<class T>
void convert(const double* a, T* b, int size);

/** Creates a directory. */
void create_directory(const std::string& dirname);

/** 
 * Deletes a directory (along with its files). 
 * Note: It does not work recursively for nested directories.
 */
void delete_directory(const std::string& dirname);

/** Expands the input MBR with the input coordinates. */
void expand_mbr(const ArraySchema* array_schema, 
                const void* coords, void* mbr);

/** Expands the input MBR with the input coordinates. */
template<class T>
void expand_mbr(const T* coords, T* mbr, int dim_num);

/** Doubles the size of the buffer. The original size is given as input. */
void expand_buffer(void*& buffer, size_t size);

/** Returns the size of the input file. */
size_t file_size(const std::string& filename);

/** Returns a list with the names of all files in the input directory. */
std::vector<std::string> get_filenames(const std::string& dirname);

/** Initializes the input MBR with the input coordinates. */
void init_mbr(const ArraySchema* array_schema, 
              const void* coords, void*& mbr);

/** Expands the input MBR with the input coordinates. */
template<class T>
void init_mbr(const T* coords, T* mbr, int dim_num);

/** True if the point lies inside the range. */
template<class T>
bool inside_range(const T* point, const T* range, int dim_num);

/** True if the input is a del value (i.e., represents deletion). */
template<class T>
bool is_del(T v);

/** Returns true if the input is an existing directory. */ 
bool is_dir(const std::string& dirname);

/** Returns true if the input is an existing file. */ 
bool is_file(const std::string& filename);

/** Returns true if the input string is an integer number. */
bool is_integer(const char* s);

/** True if the input is a null value. */
template<class T>
bool is_null(T v);

/** 
 * Returns true if the input string is a positive real number.
 * NOTE: scientific notation is currently not supported. 
 */
bool is_real(const char* s);

/** 
 * True if the input is a valid name, i.e., it contains only alphanumerics
 * and potentially '_'.
 */
bool is_valid_name(const char* s);

/** 
 * Checks the overlap between two ranges of dimensionality dim_num. 
 * Returns a pair where the first boolean indicates whether there is
 * an overlap or not, whereas the second indicates if the overlap
 * is full or not (in case the first is true).
 */
template<class T>
std::pair<bool, bool> overlap(const T* r1, const T* r2, int dim_num);

/** Returns true if the input path is an existing directory. */ 
bool path_exists(const std::string& path);

/** Wrapper of comparison function for sorting cells. */
template<typename T>
struct SmallerCol {
  /** Constructor. */
  SmallerCol(int dim_num) { dim_num_ = dim_num; }

  /** Comparison operator. */
  bool operator () (const StorageManager::Cell& a, const StorageManager::Cell& b) {
    const T* coords_a = static_cast<const T*>(a.cell_);
    const T* coords_b = static_cast<const T*>(b.cell_);

    for(int i=dim_num_-1; i>=0; --i) 
      if(coords_a[i] < coords_b[i]) 
        return true;
      else if(coords_a[i] > coords_b[i]) 
        return false;
      // else coords_a[i] == coords_b[i] --> continue
  }

  /** Number of dimension. */
  int dim_num_;
};

/** Wrapper of comparison function for sorting cells. */
template<typename T>
struct SmallerColWithId {
  /** Constructor. */
  SmallerColWithId(int dim_num) { dim_num_ = dim_num; }

  /** Comparison operator. */
  bool operator () (const StorageManager::CellWithId& a, 
                    const StorageManager::CellWithId& b) {
    if(a.id_ < b.id_)
      return true;

    if(a.id_ > b.id_)
      return false;

    // a.id_ == b.id_ --> check coordinates
    const T* coords_a = static_cast<const T*>(a.cell_);
    const T* coords_b = static_cast<const T*>(b.cell_);

    for(int i=dim_num_-1; i>=0; --i) 
      if(coords_a[i] < coords_b[i]) 
        return true;
      else if(coords_a[i] > coords_b[i]) 
        return false;
      // else coords_a[i] == coords_b[i] --> continue
  }

  /** Number of dimension. */
  int dim_num_;
};

/** Wrapper of comparison function for sorting cells. */
template<typename T>
struct SmallerRow {
  /** Constructor. */
  SmallerRow(int dim_num) { dim_num_ = dim_num; }

  /** Comparison operator. */
  bool operator () (const StorageManager::Cell& a, const StorageManager::Cell& b) {
    const T* coords_a = static_cast<const T*>(a.cell_);
    const T* coords_b = static_cast<const T*>(b.cell_);

    for(int i=0; i<dim_num_; ++i) 
      if(coords_a[i] < coords_b[i]) 
        return true;
      else if(coords_a[i] > coords_b[i]) 
        return false;
      // else coords_a[i] == coords_b[i] --> continue
  }

  /** Number of dimension. */
  int dim_num_;
};

/** Wrapper of comparison function for sorting cells. */
template<typename T>
struct SmallerRowWithId {
  /** Constructor. */
  SmallerRowWithId(int dim_num) { dim_num_ = dim_num; }

  /** Comparison operator. */
  bool operator () (const StorageManager::CellWithId& a, 
                    const StorageManager::CellWithId& b) {
    if(a.id_ < b.id_)
      return true;

    if(a.id_ > b.id_)
      return false;

    // a.id_ == b.id_ --> check coordinates
    const T* coords_a = static_cast<const T*>(a.cell_);
    const T* coords_b = static_cast<const T*>(b.cell_);

    for(int i=0; i<dim_num_; ++i) 
      if(coords_a[i] < coords_b[i]) 
        return true;
      else if(coords_a[i] > coords_b[i]) 
        return false;
      // else coords_a[i] == coords_b[i] --> continue
  }

  /** Number of dimension. */
  int dim_num_;
};

/** Wrapper of comparison function for sorting cells. */
template<typename T>
struct SmallerWith2Ids {
  /** Constructor. */
  SmallerWith2Ids(int dim_num) { dim_num_ = dim_num; }

  /** Comparison operator. */
  bool operator () (const StorageManager::CellWith2Ids& a, 
                    const StorageManager::CellWith2Ids& b) {
    if(a.tile_id_ < b.tile_id_)
      return true;

    if(a.tile_id_ > b.tile_id_)
      return false;

    if(a.cell_id_ < b.cell_id_)
      return true;

    if(a.cell_id_ > b.cell_id_)
      return false;

    // a.tile_id_ == b.tile_id_ && 
    // a.cell_id_ == b.cell_id_     --> check coordinates (row major)
    const T* coords_a = static_cast<const T*>(a.cell_);
    const T* coords_b = static_cast<const T*>(b.cell_);

    for(int i=0; i<dim_num_; ++i) 
      if(coords_a[i] < coords_b[i]) 
        return true;
      else if(coords_a[i] > coords_b[i]) 
        return false;
      // else coords_a[i] == coords_b[i] --> continue
  }

  /** Number of dimension. */
  int dim_num_;
};

#endif