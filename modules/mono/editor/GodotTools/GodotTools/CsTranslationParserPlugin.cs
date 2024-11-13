using Godot;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace GodotTools;

public partial class CsTranslationParserPlugin : EditorTranslationParserPlugin
{

    private Godot.Collections.Array<string> _msgids;
    private Godot.Collections.Array<Godot.Collections.Array> _msgidsContextPlural;

    public override string[] _GetRecognizedExtensions()
    {
        return new[] { "cs" };
    }

    public override void _ParseFile(string path, Godot.Collections.Array<string> msgids, Godot.Collections.Array<Godot.Collections.Array> msgidsContextPlural)
    {
        _msgids = msgids;
        _msgidsContextPlural = msgidsContextPlural;
        var res = ResourceLoader.Load<CSharpScript>(path, "Script");
        string text = res.SourceCode;
        SyntaxTree tree = CSharpSyntaxTree.ParseText(text);
        ParseNode(tree.GetRoot());
    }

    private void ParseNode(SyntaxNode node)
    {
        if (node is InvocationExpressionSyntax invocationExpressionSyntax)
        {
            if (invocationExpressionSyntax.Expression is MemberAccessExpressionSyntax memberAccessExpressionSyntax)
            {
                if (memberAccessExpressionSyntax.Expression is IdentifierNameSyntax identifierNameSyntax)
                {
                    if (identifierNameSyntax.Identifier.Text == "TranslationServer" && memberAccessExpressionSyntax.Name.Identifier.Text == "Translate")
                    {
                        var arguments = invocationExpressionSyntax.ArgumentList.Arguments;
                        if (arguments.Count == 1 && arguments[0].Expression is LiteralExpressionSyntax literalExpression && literalExpression.Token.Value is string value)
                        {
                            _msgids.Add(value);
                        }
                        if (arguments.Count == 2 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral && messageLiteral.Token.Value is string message && arguments[1].Expression is LiteralExpressionSyntax contextLiteral && contextLiteral.Token.Value is string context)
                        {
                            _msgidsContextPlural.Add(new Godot.Collections.Array { message, context, "" });
                        }
                    }
                    if (identifierNameSyntax.Identifier.Text == "TranslationServer" && memberAccessExpressionSyntax.Name.Identifier.Text == "TranslatePlural")
                    {
                        var arguments = invocationExpressionSyntax.ArgumentList.Arguments;
                        if (arguments.Count == 3 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral1 && messageLiteral1.Token.Value is string message1 && arguments[1].Expression is LiteralExpressionSyntax pluralLiteral1 && pluralLiteral1.Token.Value is string plural1)
                        {
                            _msgidsContextPlural.Add(new Godot.Collections.Array { message1, "", plural1 });
                        }
                        if (arguments.Count == 4 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral2 && messageLiteral2.Token.Value is string message2 && arguments[1].Expression is LiteralExpressionSyntax pluralLiteral2 && pluralLiteral2.Token.Value is string plural2 && arguments[3].Expression is LiteralExpressionSyntax contextLiteral && contextLiteral.Token.Value is string context)
                        {
                            _msgidsContextPlural.Add(new Godot.Collections.Array { message2, context, plural2 });
                        }
                    }
                }
                switch (memberAccessExpressionSyntax.Name.Identifier.Text)
                {
                    case "Tr":
                        {
                            var arguments = invocationExpressionSyntax.ArgumentList.Arguments;
                            if (arguments.Count == 1 && arguments[0].Expression is LiteralExpressionSyntax literalExpression && literalExpression.Token.Value is string value)
                            {
                                _msgids.Add(value);
                            }
                            if (arguments.Count == 2 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral && messageLiteral.Token.Value is string message && arguments[1].Expression is LiteralExpressionSyntax contextLiteral && contextLiteral.Token.Value is string context)
                            {
                                _msgidsContextPlural.Add(new Godot.Collections.Array { message, context, "" });
                            }

                            break;
                        }
                    case "TrN":
                        {
                            var arguments = invocationExpressionSyntax.ArgumentList.Arguments;
                            if (arguments.Count == 3 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral1 && messageLiteral1.Token.Value is string message1 && arguments[1].Expression is LiteralExpressionSyntax pluralLiteral1 && pluralLiteral1.Token.Value is string plural1)
                            {
                                _msgidsContextPlural.Add(new Godot.Collections.Array { message1, "", plural1 });
                            }

                            if (arguments.Count == 4 && arguments[0].Expression is LiteralExpressionSyntax messageLiteral2 && messageLiteral2.Token.Value is string message2 && arguments[1].Expression is LiteralExpressionSyntax pluralLiteral2 && pluralLiteral2.Token.Value is string plural2 && arguments[3].Expression is LiteralExpressionSyntax contextLiteral && contextLiteral.Token.Value is string context)
                            {
                                _msgidsContextPlural.Add(new Godot.Collections.Array { message2, context, plural2 });
                            }

                            break;
                        }
                }
            }
        }

        foreach (var childNode in node.ChildNodes())
        {
            ParseNode(childNode);
        }
    }
}
