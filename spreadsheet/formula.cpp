#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "FormulaAST.h"
#include "common.h"

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    using namespace std::string_view_literals;

    switch (category_) {
        case Category::Arithmetic:
            return "#ARITHM!"sv;
        case Category::Ref:
            return "#REF!"sv;
        case Category::Value:
            return "#VALUE!"sv;
    }

    return ""sv;
}

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << fe.ToString();
}

namespace {

class Formula final : public FormulaInterface {
private:
    template <typename... Ts>
    struct Overload : std::decay_t<Ts>... {
        using std::decay_t<Ts>::operator()...;
    };

    template <typename... Ts>
    Overload(Ts...) -> Overload<Ts...>;

public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)),
          referenced_cells_(ast_.GetReferencedCells().cbegin(),
                            ast_.GetReferencedCells().cend()) {
        //
        referenced_cells_.erase(
            std::unique(referenced_cells_.begin(), referenced_cells_.end()),
            referenced_cells_.end());

        const auto kPred = [](const Position ref_cell) -> bool {
            return !ref_cell.IsValid();
        };

        if (const auto kCellit = std::find_if(referenced_cells_.cbegin(),
                                              referenced_cells_.cend(), kPred);
            kCellit != referenced_cells_.cend()) {
            //
            throw FormulaException("invalid cell position" +
                                   kCellit->ToString());
        }
    } catch (const FormulaException&) {
        throw;
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    } catch (...) {
        throw FormulaException("");
    }

    [[nodiscard]] Value Evaluate(const SheetInterface& sheet) const override {
        const auto kLookup = [&sheet](Position pos) -> double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            const auto* cell = sheet.GetCell(pos);
            if (nullptr == cell) {
                return 0.0;
            }

            return std::visit(
                Overload{
                    [](double d) -> double { return d; },
                    [](const std::string& s) -> double {
                        if (s.empty()) {
                            return 0.0;
                        }

                        try {
                            size_t pos_index = 0;
                            const double kResult = std::stod(s, &pos_index);

                            if (pos_index != s.size()) {
                                throw FormulaError(
                                    FormulaError::Category::Value);
                            }

                            return kResult;
                        } catch (...) {
                            throw FormulaError(FormulaError::Category::Value);
                        };
                    },
                    [](FormulaError fe) -> double { throw fe; },
                },
                cell->GetValue());
        };

        try {
            return ast_.Execute(kLookup);
        } catch (const FormulaError& fe) {
            return fe;
        }
    }

    [[nodiscard]] std::string GetExpression() const override {
        std::stringstream result{};

        ast_.PrintFormula(result);

        return result.str();
    }

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }

private:
    FormulaAST ast_;

    std::vector<Position> referenced_cells_;
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
