using System.Collections.Generic;
using System.IO;
using System.Linq;
using Godot;
using Godot.Collections;
using GodotTools.Internals;
using Microsoft.Build.Evaluation;
using Microsoft.Build.Execution;
using Microsoft.Build.Locator;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace GodotTools;

public partial class CsTranslationParserPlugin : EditorTranslationParserPlugin
{

    private List<MetadataReference> _projectReferences;

    public override string[] _GetRecognizedExtensions()
    {
        return new[] { "cs" };
    }

    public override void _ParseFile(string path, Array<string> msgids, Array<Array> msgidsContextPlural)
    {
        if (_projectReferences == null)
        {
            _projectReferences = GetProjectReferences(GodotSharpDirs.ProjectCsProjPath);
            var references = System.AppDomain.CurrentDomain.GetAssemblies()
                .Where(a => !a.IsDynamic)
                .Where(a => a.Location != "")
                .Select(a => MetadataReference.CreateFromFile(a.Location))
                .Cast<MetadataReference>();
            _projectReferences.AddRange(references);
        }
        var res = ResourceLoader.Load<CSharpScript>(path, "Script");
        var text = res.SourceCode;
        var tree = CSharpSyntaxTree.ParseText(text);

        var compilation = CSharpCompilation.Create("TranslationParser", options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary))
            .AddReferences(_projectReferences)
            .AddSyntaxTrees(tree);

        var semanticModel = compilation.GetSemanticModel(tree);
        foreach (var syntaxNode in tree.GetRoot().DescendantNodes().Where(node => node is InvocationExpressionSyntax))
        {

            var invocation = (InvocationExpressionSyntax)syntaxNode;
            SymbolInfo? symbolInfo = null;
            if (invocation.Expression is IdentifierNameSyntax identifierNameSyntax)
            {
                symbolInfo = semanticModel.GetSymbolInfo(identifierNameSyntax);

            }
            if (invocation.Expression is MemberAccessExpressionSyntax memberAccessExpressionSyntax && memberAccessExpressionSyntax.Name is IdentifierNameSyntax nameSyntax)
            {
                symbolInfo = semanticModel.GetSymbolInfo(nameSyntax);
            }

            var methodSymbol = symbolInfo?.Symbol as IMethodSymbol;
            if (methodSymbol == null)
                continue;

            if (methodSymbol.Name == "Translate" &&
                methodSymbol.ContainingType.ToDisplayString() == "Godot.TranslationServer")
            {
                AddMsg(invocation.ArgumentList.Arguments, semanticModel, msgids, msgidsContextPlural);
            }

            if (methodSymbol.Name == "TranslatePlural" &&
                methodSymbol.ContainingType.ToDisplayString() == "Godot.TranslationServer")
            {
                AddPluralMsg(invocation.ArgumentList.Arguments, semanticModel, msgidsContextPlural);
            }

            if ((methodSymbol.Name == "Tr" || methodSymbol.Name == "TrN")
                && methodSymbol.MethodKind == MethodKind.Ordinary)
            {
                var receiverType = methodSymbol.ReceiverType ?? methodSymbol.ContainingType;

                if (receiverType != null && InheritsFromGodotObject(receiverType))
                {
                    if (methodSymbol.Name == "Tr")
                    {
                        AddMsg(invocation.ArgumentList.Arguments, semanticModel, msgids, msgidsContextPlural);
                    }
                    else
                    {
                        AddPluralMsg(invocation.ArgumentList.Arguments, semanticModel, msgidsContextPlural);
                    }
                }
            }
        }
    }

    private bool InheritsFromGodotObject(ITypeSymbol typeSymbol)
    {
        while (typeSymbol != null)
        {
            if (typeSymbol.ToDisplayString() == "Godot.GodotObject")
                return true;
#pragma warning disable CS8600
            typeSymbol = typeSymbol.BaseType;
#pragma warning restore CS8600
        }
        return false;
    }

    private void AddMsg(SeparatedSyntaxList<ArgumentSyntax> arguments, SemanticModel semanticModel, Array<string> msgids, Array<Array> msgidsContextPlural)
    {
        switch (arguments.Count)
        {
            case 1:
            {
                var argExpr = arguments[0].Expression;
                var constantValue = semanticModel.GetConstantValue(argExpr);

                if (constantValue.HasValue && constantValue.Value is string message)
                {
                    msgids.Add(message);
                }

                break;
            }
            case 2:
            {
                var msgExpr = arguments[0].Expression;
                var ctxExpr = arguments[1].Expression;

                var msgValue = semanticModel.GetConstantValue(msgExpr);
                var ctxValue = semanticModel.GetConstantValue(ctxExpr);

                if (msgValue.HasValue && msgValue.Value is string message &&
                    ctxValue.HasValue && ctxValue.Value is string context)
                {
                    msgidsContextPlural.Add(new Array { message, context, "" });
                }

                break;
            }
        }
    }

    private void AddPluralMsg(SeparatedSyntaxList<ArgumentSyntax> arguments, SemanticModel semanticModel, Array<Array> msgidsContextPlural)
    {
        var singularExpr = arguments[0].Expression;
        var pluralExpr = arguments[1].Expression;

        var singularValue = semanticModel.GetConstantValue(singularExpr);
        var pluralValue = semanticModel.GetConstantValue(pluralExpr);

        if (!singularValue.HasValue || singularValue.Value is not string singular ||
            !pluralValue.HasValue || pluralValue.Value is not string plural) return;

        var context = "";
        if (arguments.Count == 4)
        {
            var ctxExpr = arguments[3].Expression;
            var ctxValue = semanticModel.GetConstantValue(ctxExpr);
            if (ctxValue.HasValue && ctxValue.Value is string ctx)
            {
                context = ctx;
            }
        }
        msgidsContextPlural.Add(new Array { singular, context, plural });
    }

    private List<MetadataReference> GetProjectReferences(string projectPath)
    {
        if (!MSBuildLocator.IsRegistered)
        {
            MSBuildLocator.RegisterDefaults();
        }

        var referencePaths = GetProjectReferencePaths(projectPath);

        var metadataReferences = new List<MetadataReference>();
        foreach (var dllPath in referencePaths)
        {
            if (File.Exists(dllPath))
            {
                var metadataReference = MetadataReference.CreateFromFile(dllPath);
                metadataReferences.Add(metadataReference);
            }
        }

        return metadataReferences;
    }

    private List<string> GetProjectReferencePaths(string projectPath)
    {
        var referencePaths = new List<string>();

        var projectCollection = new ProjectCollection();
        var project = projectCollection.LoadProject(projectPath);

        project.SetProperty("Configuration", "Debug");
        project.SetProperty("Platform", "Any CPU");

        var buildParameters = new BuildParameters(projectCollection);
        var buildRequest = new BuildRequestData(project.FullPath, project.GlobalProperties, null, new[] { "GetTargetPath" }, null);
        var buildResult = BuildManager.DefaultBuildManager.Build(buildParameters, buildRequest);

        if (buildResult.OverallResult == BuildResultCode.Success)
        {
            foreach (var item in buildResult.ResultsByTarget["GetTargetPath"].Items)
            {
                referencePaths.Add(item.ItemSpec);
            }
        }

        projectCollection.UnloadAllProjects();
        projectCollection.Dispose();

        return referencePaths;
    }
}
