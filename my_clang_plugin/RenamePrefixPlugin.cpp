#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

namespace {
class RenameVisitor : public RecursiveASTVisitor<RenameVisitor> {
public:
  RenameVisitor(Rewriter &R, ASTContext &C) : TheRewriter(R), Context(C) {}

  bool VisitVarDecl(VarDecl *D) {
    if (D->isImplicit()) return true;

    SourceLocation Loc = D->getLocation();
    if (Loc.isInvalid() || Context.getSourceManager().isInSystemHeader(Loc)) return true;

    std::string prefix;
    if (D->isStaticLocal())
      prefix = "static_";
    else if (D->hasGlobalStorage() && !isa<ParmVarDecl>(D))
      prefix = "global_";
    else if (isa<ParmVarDecl>(D))
      prefix = "param_";
    else
      prefix = "local_";
    
    std::string newName = prefix + D->getNameAsString();
    TheRewriter.ReplaceText(D->getLocation(), D->getName().size(), newName);

    return true;
  }

  bool VisitUnaryOperator(UnaryOperator *U) {
    if (U->getOpcode() == UO_PreInc || U->getOpcode() == UO_PostInc) {
      Expr *subExpr = U->getSubExpr();
      if (auto *D = dyn_cast<DeclRefExpr>(subExpr)) {
        VarDecl *varDecl = dyn_cast<VarDecl>(D->getDecl()); // получаем ссылку на переменную
        if (varDecl) {
          std::string newName = getPrefixedVarName(varDecl); // заменяем имя переменной
          TheRewriter.ReplaceText(D->getLocation(), D->getNameInfo().getName().getAsString().size(), newName);
        }
      }
    }

    return true;
  }
  

  bool VisitReturnStmt(ReturnStmt *RS) {
    Expr *returnExpr = RS->getRetValue();
    if (returnExpr)
      this->TraverseStmt(returnExpr); // рекурсивно обрабатываем выражение

    return true;
  }
  
  bool VisitDeclRefExpr(DeclRefExpr *D) {
    if (VarDecl *varDecl = dyn_cast<VarDecl>(D->getDecl())) {
      std::string newName = getPrefixedVarName(varDecl); // новое имя для переменной
      TheRewriter.ReplaceText(SourceRange(D->getLocation(), D->getLocation().getLocWithOffset(D->getNameInfo().getName().getAsString().size())), newName);// текст с правильным диапазоном
    }
    return true;
  }

private:
  Rewriter &TheRewriter;
  ASTContext &Context;

  std::string getPrefixedVarName(VarDecl *D) {
    std::string prefix;
    if (D->isStaticLocal())
      prefix = "static_";
    else if (D->hasGlobalStorage() && !isa<ParmVarDecl>(D))
      prefix = "global_";
    else if (isa<ParmVarDecl>(D))
      prefix = "param_";
    else
      prefix = "local_";

    return prefix + D->getNameAsString();
  }
};

class RenameConsumer : public ASTConsumer {
public:
  RenameConsumer(Rewriter &R, ASTContext &C) : Visitor(R, C) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  RenameVisitor Visitor;
};

class RenameAction : public PluginASTAction {
protected:
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<RenameConsumer>(TheRewriter, CI.getASTContext());
  }

  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
  }

private:
  Rewriter TheRewriter;
};
}

static FrontendPluginRegistry::Add<RenameAction>
    X("rename-prefixes", "Add variable name prefixes");
