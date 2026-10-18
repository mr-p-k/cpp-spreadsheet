#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
private:
    enum class PrintType : char {
        TEXT,
        VALUE,
    };

    struct PositionHasher {
        size_t operator()(Position pos) const;
    };

    void AddIndexes(Position pos);

    void EraseIndexes(Position pos);

    void PrintSheet(std::ostream& out, PrintType type) const;

    template <typename InputIt>
    void CheckCircularDependency(InputIt first, InputIt last,
                                 Position pos) const;

public:
    Sheet() = default;

    Sheet(const Sheet&) = delete;

    Sheet(Sheet&&) = default;

    Sheet& operator=(const Sheet&) = delete;

    Sheet& operator=(Sheet&&) = default;

    ~Sheet() override = default;

    void SetCell(Position pos, std::string text) override;

    [[nodiscard]] const CellInterface* GetCell(Position pos) const override;

    [[nodiscard]] CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    [[nodiscard]] Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;

    void PrintTexts(std::ostream& output) const override;

    [[nodiscard]] const Cell* GetConcreteCell(Position pos) const;

    [[nodiscard]] Cell* GetConcreteCell(Position pos);

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> sheet_;

    std::map<int, std::set<int>> rows_index_;

    std::map<int, std::set<int>> cols_index_;
};

template <typename InputIt>
void Sheet::CheckCircularDependency(InputIt first, InputIt last,
                                    Position pos) const {
    std::stack<Position, std::vector<Position>> to_visit;
    std::unordered_set<Position, PositionHasher> visited;

    for (auto it = first; it != last; ++it) {
        to_visit.emplace(*it);

        while (!to_visit.empty()) {
            const Position kCurr = to_visit.top();
            to_visit.pop();

            if (kCurr == pos) {
                throw CircularDependencyException(
                    "Circular dependency detected");
            }

            if (visited.insert(kCurr).second) {
                const auto* cell = GetCell(kCurr);

                if (nullptr != cell) {
                    for (const auto kPos : cell->GetReferencedCells()) {
                        to_visit.emplace(kPos);
                    }
                }
            }
        }
    }
}