#pragma once

#include <string>
#include <vector>

#include "src/carnot/proto/plan.pb.h"
#include "src/common/types/types.pb.h"

namespace pl {
namespace carnot {
namespace plan {

using ColTypeArray = std::vector<types::DataType>;
using ColNameArray = std::vector<std::string>;

/**
 * Relation tracks columns/types for a given table/operator
 */
class Relation {
 public:
  Relation();
  // Constructor for Relation that initializes with a list of column types.
  explicit Relation(const ColTypeArray &col_types, const ColNameArray &col_names);

  // Get the column types.
  const ColTypeArray &col_types() const { return col_types_; }
  // Get the column names.
  const ColNameArray &col_names() const { return col_names_; }

  // Returns the number of columns.
  size_t NumColumns() const;

  // Add a column to the relation.
  void AddColumn(const types::DataType &col_type, const std::string &col_name);

  // Check if the column at idx exists.
  bool HasColumn(size_t idx) const;

  types::DataType GetColumnType(size_t idx) const;
  std::string GetColumnName(size_t idx) const;

  // Get the debug string of this relation.
  std::string DebugString() const;

 private:
  ColTypeArray col_types_;
  ColNameArray col_names_;
};

}  // namespace plan
}  // namespace carnot
}  // namespace pl
