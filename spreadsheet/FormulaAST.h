#pragma once

#include <forward_list>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// #include "FormulaLexer.h"
#include "common.h"

namespace ASTImpl {

class Expr;

}  // namespace ASTImpl

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
public:
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr,
                        std::forward_list<Position> referenced_cells);

    FormulaAST(const FormulaAST&) = delete;

    FormulaAST(FormulaAST&&) = default;

    FormulaAST& operator=(const FormulaAST&) = delete;

    FormulaAST& operator=(FormulaAST&&) = default;

    ~FormulaAST();

    [[nodiscard]] double Execute(
        const std::function<double(Position)>& lookup) const;

    void Print(std::ostream& out) const;

    void PrintFormula(std::ostream& out) const;

    void PrintCells(std::ostream& out) const;

    [[nodiscard]] const std::forward_list<Position>& GetReferencedCells() const;

private:
    std::unique_ptr<ASTImpl::Expr> root_expr_;

    std::forward_list<Position> referenced_cells_;
};

FormulaAST ParseFormulaAST(std::istream& in);

FormulaAST ParseFormulaAST(const std::string& in_str);