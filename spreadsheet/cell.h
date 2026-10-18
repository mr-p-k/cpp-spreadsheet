#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell final : public CellInterface {
private:
    class Empty {};

    class Text {
    public:
        explicit Text(std::string value);

        [[nodiscard]] Value GetValue() const;

        [[nodiscard]] std::string GetText() const;

    private:
        std::string value_;
    };

    class Formula {
    public:
        explicit Formula(std::string expression);

        [[nodiscard]] Value GetValue(const SheetInterface& sheet) const;

        [[nodiscard]] std::string GetText() const;

        [[nodiscard]] std::vector<Position> GetReferencedCells() const;

        [[nodiscard]] bool HasCache() const;

        void ResetCache();

    private:
        std::unique_ptr<FormulaInterface> formula_;

        mutable std::optional<FormulaInterface::Value> cache_;
    };

    using CellVariant = std::variant<Empty, Text, Formula>;

    template <typename... Ts>
    struct Overload : std::decay_t<Ts>... {
        using std::decay_t<Ts>::operator()...;
    };

    template <typename... Ts>
    Overload(Ts...) -> Overload<Ts...>;

public:
    explicit Cell(Sheet& sheet);

    Cell(const Cell&) = delete;

    Cell(Cell&&) = delete;

    Cell& operator=(const Cell&) = delete;

    Cell& operator=(Cell&&) = delete;

    ~Cell() override = default;

    void Set(std::string text);

    void Clear();

    void InvalidateCache();

    void ResetCache();

    [[nodiscard]] Value GetValue() const override;

    [[nodiscard]] std::string GetText() const override;

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;

    [[nodiscard]] const std::unordered_set<Cell*>& GetDependentCellNodes()
        const;

    [[nodiscard]] const std::unordered_set<Cell*>& GetReferencedCellNodes()
        const;

    void AddDependentCellNode(Cell* cell);

    void RemoveDependentCellNode(Cell* cell);

    void SetReferencedCellNode(std::unordered_set<Cell*> cells);

    void ClearReferencedCellNodes();

private:
    CellVariant impl_;

    Sheet& sheet_;

    std::unordered_set<Cell*> dependent_cells_;

    std::unordered_set<Cell*> referenced_cells_;
};