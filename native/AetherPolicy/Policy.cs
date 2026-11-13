using System.Runtime.CompilerServices;
using System.Text;

namespace AetherPolicy;

public static unsafe class Policy
{
    private static readonly UTF8Encoding StrictUtf8 = new(false, true);

    // Stable C ABI consumed directly by the C++ message ingress path.
    // 0 = accepted, 1 = malformed/oversized input, 2 = invalid pointer/length.
    [UnmanagedCallersOnly(EntryPoint = "aether_policy_validate_message",
        CallConvs = new[] { typeof(CallConvCdecl) })]
    public static int ValidateMessage(byte* data, int length)
    {
        if (data == null || length < 0 || length > 4096)
            return 2;

        try
        {
            var bytes = new ReadOnlySpan<byte>(data, length);
            var text = StrictUtf8.GetString(bytes);
            foreach (var character in text)
            {
                if (character == '\0')
                    return 1;
                if (char.IsControl(character) && character != '\n' && character != '\r' && character != '\t')
                    return 1;
            }
            return 0;
        }
        catch (DecoderFallbackException)
        {
            return 1;
        }
    }
}
