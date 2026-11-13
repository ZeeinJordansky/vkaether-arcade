using System.Buffers;
using System.Runtime.CompilerServices;
using System.Text;

namespace AetherPolicy;

public static unsafe class Policy
{
    // Stable C ABI consumed directly by the C++ message ingress path.
    // 0 = accepted, 1 = malformed input, 2 = invalid pointer/length.
    [UnmanagedCallersOnly(EntryPoint = "aether_policy_validate_message",
        CallConvs = new[] { typeof(CallConvCdecl) })]
    public static int ValidateMessage(byte* data, int length)
    {
        if (data == null || length < 0 || length > 4096)
            return 2;

        // Decode one Unicode scalar at a time. This validates UTF-8 and checks
        // controls without allocating an intermediate managed string per event.
        var remaining = new ReadOnlySpan<byte>(data, length);
        while (!remaining.IsEmpty)
        {
            var status = Rune.DecodeFromUtf8(remaining, out var rune, out var consumed);
            if (status != OperationStatus.Done || consumed <= 0)
                return 1;

            if (Rune.IsControl(rune) && rune.Value != '\n' && rune.Value != '\r' && rune.Value != '\t')
                return 1;

            remaining = remaining[consumed..];
        }
        return 0;
    }
}
