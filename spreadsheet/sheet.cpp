#include "sheet.h"

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>

#include "cell.h"
#include "common.h"

using namespace std::literals;

namespace {

void CheckPosition(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position is out of bounds: " +
                                       pos.ToString());
    }
}

void CorrectErase(int where, int what, std::map<int, std::set<int>>& index) {
    if (auto const kKeyIt = index.find(where); kKeyIt != index.end()) {
        kKeyIt->second.erase(what);
        if (kKeyIt->second.empty()) {
            index.erase(kKeyIt);
        }
    }
}

template <typename... Ts>  // NOLINTNEXTLINE(misc-multiple-inheritance)
struct Overload : std::decay_t<Ts>... {
    using std::decay_t<Ts>::operator()...;
};
template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

std::ostream& operator<<(std::ostream& out, const CellInterface::Value& value) {
    std::visit(Overload{
                   [&out](const std::string& s) { out << s; },
                   [&out](double d) { out << d; },
                   [&out](const FormulaError& e) { out << e; },
               },
               value);

    return out;
}

}  // namespace

void Sheet::AddIndexes(Position pos) {
    rows_index_[pos.row].emplace(pos.col);
    cols_index_[pos.col].emplace(pos.row);
}

void Sheet::EraseIndexes(Position pos) {
    CorrectErase(pos.row, pos.col, rows_index_);

    CorrectErase(pos.col, pos.row, cols_index_);
}

void Sheet::PrintSheet(std::ostream& out, PrintType type) const {
    const auto kPrintableArea = GetPrintableSize();

    if (kPrintableArea.cols == 0 || kPrintableArea.rows == 0) {
        return;
    }

    for (int row = 0; row < kPrintableArea.rows; ++row) {
        for (int col = 0; col < kPrintableArea.cols; ++col) {
            const Position kPos{row, col};

            const auto* cell = GetCell(kPos);

            if (nullptr != cell) {
                switch (type) {
                    case PrintType::VALUE:
                        out << cell->GetValue();
                        // out << "kokos";
                        break;
                    case PrintType::TEXT:
                        out << cell->GetText();
                        break;
                    default:
                        break;
                }
            }

            if (col < kPrintableArea.cols - 1) {
                out << '\t';
            }
        }

        out << '\n';
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    auto tmp_cell = std::make_unique<Cell>(*this);
    tmp_cell->Set(std::move(text));

    const auto& ref_cells = tmp_cell->GetReferencedCells();

    CheckCircularDependency(ref_cells.cbegin(), ref_cells.cend(), pos);

    auto [cell_it, is_ok] = sheet_.try_emplace(pos);

    if (!is_ok) {
        auto& old_cell = cell_it->second;

        old_cell->InvalidateCache();

        for (Cell* node_ptr : old_cell->GetReferencedCellNodes()) {
            if (nullptr != node_ptr) {
                node_ptr->RemoveDependentCellNode(old_cell.get());
            }
        }
        old_cell->ClearReferencedCellNodes();

        for (Cell* node_ptr : old_cell->GetDependentCellNodes()) {
            if (nullptr != node_ptr) {
                tmp_cell->AddDependentCellNode(node_ptr);
            }
        }
    }

    cell_it->second = std::move(tmp_cell);

    std::unordered_set<Cell*> ref_cell_nodes;
    for (const auto& cell_pos : cell_it->second->GetReferencedCells()) {
        if (Cell* ref_cell_ptr = GetConcreteCell(cell_pos);
            nullptr != ref_cell_ptr) {
            //
            ref_cell_nodes.insert(ref_cell_ptr);
            ref_cell_ptr->AddDependentCellNode(cell_it->second.get());
        }
    }

    cell_it->second->SetReferencedCellNode(std::move(ref_cell_nodes));

    if (!cell_it->second->GetText().empty()) {
        AddIndexes(pos);
    } else {
        EraseIndexes(pos);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos);

    auto const kCellIt = sheet_.find(pos);

    return (kCellIt != sheet_.cend()) ? kCellIt->second.get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(
        static_cast<const Sheet&>(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);

    if (0 < sheet_.erase(pos)) {
        EraseIndexes(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    Size result{};

    if (rows_index_.empty() || cols_index_.empty()) {
        return result;
    }

    result.rows = rows_index_.crbegin()->first + 1;
    result.cols = cols_index_.crbegin()->first + 1;

    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintSheet(output, PrintType::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintSheet(output, PrintType::TEXT);
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    CheckPosition(pos);

    const auto kCellIt = sheet_.find(pos);

    return (kCellIt != sheet_.cend()) ? kCellIt->second.get() : nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    CheckPosition(pos);

    auto [cell_it, is_ok] = sheet_.try_emplace(pos);

    if (is_ok) {
        cell_it->second = std::make_unique<Cell>(*this);
    }

    return cell_it->second.get();
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

size_t Sheet::PositionHasher::operator()(Position pos) const {
    const std::hash<int> kHashInt;
    const int kSalt = 37;

    return kHashInt(pos.row + (pos.col * kSalt));
}
