#include "cell.h"

#include <cassert>
#include <stack>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "common.h"
#include "formula.h"
#include "sheet.h"

Cell::Cell(Sheet& sheet) : impl_(Empty{}), sheet_(sheet) {
}

void Cell::Set(std::string text) {
    if (text == GetText()) {
        return;
    }

    if (text.empty()) {
        impl_ = Empty{};
    } else if (FORMULA_SIGN == text.front()) {
        if (1 < text.length()) {
            impl_ = Formula{text.substr(1)};
        } else {
            impl_ = Text{std::move(text)};
        }
    } else {
        impl_ = Text{std::move(text)};
    }
}

void Cell::Clear() {
    Set("");
}

void Cell::InvalidateCache() {
    std::stack<Cell*, std::vector<Cell*>> to_visit;
    std::unordered_set<Cell*> visited;

    to_visit.push(this);

    while (!to_visit.empty()) {
        auto* curr = to_visit.top();
        to_visit.pop();

        if (visited.insert(curr).second) {
            curr->ResetCache();

            for (auto* cell : curr->dependent_cells_) {
                to_visit.push(cell);
            }
        }
    }
}

void Cell::ResetCache() {
    if (auto* formula = std::get_if<Formula>(&impl_)) {
        formula->ResetCache();
    }
}

Cell::Value Cell::GetValue() const {
    return std::visit(
        Overload{
            [](const Empty& e) { return e.GetValue(); },
            [](const Text& t) { return t.GetValue(); },
            [this](const Formula& f) { return f.GetValue(sheet_); },
        },
        impl_);
}

std::string Cell::GetText() const {
    return std::visit(Overload{
                          [](const Empty& e) { return e.GetText(); },
                          [](const Text& t) { return t.GetText(); },
                          [](const Formula& f) { return f.GetText(); },
                      },
                      impl_);
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (const auto* formula = std::get_if<Formula>(&impl_);
        nullptr != formula) {
        //
        return formula->GetReferencedCells();
    }

    return {};
}

const std::unordered_set<Cell*>& Cell::GetDependentCellNodes() const {
    return dependent_cells_;
}

const std::unordered_set<Cell*>& Cell::GetReferencedCellNodes() const {
    return referenced_cells_;
}

void Cell::AddDependentCellNode(Cell* cell) {
    if (nullptr != cell) {
        dependent_cells_.insert(cell);
    }
}

void Cell::RemoveDependentCellNode(Cell* cell) {
    if (nullptr != cell) {
        dependent_cells_.erase(cell);
    }
}

void Cell::SetReferencedCellNode(std::unordered_set<Cell*> cells) {
    referenced_cells_ = std::move(cells);
}

void Cell::ClearReferencedCellNodes() {
    referenced_cells_.clear();
}

CellInterface::Value Cell::Empty::GetValue() const {
    return 0.0;
}

std::string Cell::Empty::GetText() const {
    return {};
}

Cell::Text::Text(std::string value) : value_(std::move(value)) {
}

Cell::Value Cell::Text::GetValue() const {
    return (ESCAPE_SIGN == value_.front() ? value_.substr(1) : value_);
}

std::string Cell::Text::GetText() const {
    return value_;
}

Cell::Formula::Formula(std::string expression)
    : formula_(ParseFormula(std::move(expression))) {
}

Cell::Value Cell::Formula::GetValue(const SheetInterface& sheet) const {
    return std::visit(
        Overload{
            [](double value) -> Cell::Value { return value; },
            [](const FormulaError& e) -> Cell::Value { return e; },
        },
        formula_->Evaluate(sheet));
}

std::string Cell::Formula::GetText() const {
    using namespace std::string_literals;

    return "="s + formula_->GetExpression();
}

std::vector<Position> Cell::Formula::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

bool Cell::Formula::HasCache() const {
    return cache_.has_value();
}

void Cell::Formula::ResetCache() {
    cache_.reset();
}
