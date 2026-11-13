//! Small, stable C ABI boundary for performance-sensitive primitives.
//! Keep this crate dependency-free so builds remain reproducible.

use std::ffi::c_int;

/// Adds two signed 64-bit integers.
///
/// Returns 0 on success and writes the result to `out`.
/// Returns 1 for a null output pointer and 2 on arithmetic overflow.
#[no_mangle]
pub extern "C" fn vkaether_rust_add(a: i64, b: i64, out: *mut i64) -> c_int {
    if out.is_null() {
        return 1;
    }

    match a.checked_add(b) {
        Some(value) => {
            // SAFETY: null was rejected above; the caller owns a writable i64.
            unsafe { out.write(value) };
            0
        }
        None => 2,
    }
}


/// Resolves rock-paper-scissors using 0=rock, 1=paper, 2=scissors.
/// Returns 0 for a draw, 1 for a player win, 2 for a bot win, -1 for invalid input.
#[no_mangle]
pub extern "C" fn vkaether_rust_rps(player: c_int, bot: c_int) -> c_int {
    if !(0..=2).contains(&player) || !(0..=2).contains(&bot) {
        return -1;
    }
    if player == bot {
        return 0;
    }
    if (player + 1) % 3 == bot {
        2
    } else {
        1
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn resolves_rock_paper_scissors() {
        assert_eq!(vkaether_rust_rps(0, 0), 0);
        assert_eq!(vkaether_rust_rps(1, 0), 1);
        assert_eq!(vkaether_rust_rps(0, 1), 2);
        assert_eq!(vkaether_rust_rps(3, 0), -1);
    }

    #[test]
    fn adds_values_without_overflow() {
        let mut result = 0;
        assert_eq!(vkaether_rust_add(20, 22, &mut result), 0);
        assert_eq!(result, 42);
    }

    #[test]
    fn rejects_overflow() {
        let mut result = 0;
        assert_eq!(vkaether_rust_add(i64::MAX, 1, &mut result), 2);
    }

    #[test]
    fn rejects_null_output() {
        assert_eq!(vkaether_rust_add(1, 2, std::ptr::null_mut()), 1);
    }

    #[test]
    fn checked_game_math_handles_boundaries() {
        let mut value = 0_i64;
        assert_eq!(vkaether_rust_mul(6, 7, &mut value), 0);
        assert_eq!(value, 42);
        assert_eq!(vkaether_rust_mul(i64::MAX, 2, &mut value), 2);
        assert_eq!(vkaether_rust_div(10, 0, &mut value), 2);
        assert_eq!(vkaether_rust_div(i64::MIN, -1, &mut value), 3);
        assert_eq!(vkaether_rust_pow(3, 4, &mut value), 0);
        assert_eq!(value, 81);

        let mut factorial = 0_u64;
        assert_eq!(vkaether_rust_factorial(20, &mut factorial), 0);
        assert_eq!(factorial, 2_432_902_008_176_640_000);
        assert_eq!(vkaether_rust_factorial(21, &mut factorial), 4);
        assert_eq!(vkaether_rust_is_prime(97), 1);
        assert_eq!(vkaether_rust_is_prime(100), 0);
        assert_eq!(vkaether_rust_is_prime(18_446_744_073_709_551_557), 1);
    }
}

/// Checked multiplication. Status: 0=success, 1=null output, 2=overflow.
#[no_mangle]
pub extern "C" fn vkaether_rust_mul(a: i64, b: i64, out: *mut i64) -> c_int {
    if out.is_null() { return 1; }
    match a.checked_mul(b) {
        Some(value) => { unsafe { out.write(value) }; 0 }
        None => 2,
    }
}

/// Checked integer division. Status: 0=success, 1=null output, 2=zero divisor, 3=overflow.
#[no_mangle]
pub extern "C" fn vkaether_rust_div(a: i64, b: i64, out: *mut i64) -> c_int {
    if out.is_null() { return 1; }
    match a.checked_div(b) {
        Some(value) => { unsafe { out.write(value) }; 0 }
        None if b == 0 => 2,
        None => 3,
    }
}

/// Checked integer exponentiation. Exponent must be in 0..=63.
#[no_mangle]
pub extern "C" fn vkaether_rust_pow(base: i64, exponent: u32, out: *mut i64) -> c_int {
    if out.is_null() { return 1; }
    if exponent > 63 { return 4; }
    match base.checked_pow(exponent) {
        Some(value) => { unsafe { out.write(value) }; 0 }
        None => 2,
    }
}

/// Factorial in the exact i64-safe domain 0..=20.
#[no_mangle]
pub extern "C" fn vkaether_rust_factorial(n: u32, out: *mut u64) -> c_int {
    if out.is_null() { return 1; }
    if n > 20 { return 4; }
    let mut value = 1_u64;
    for factor in 2..=n { value *= u64::from(factor); }
    unsafe { out.write(value) };
    0
}

fn modular_multiply(a: u64, b: u64, modulus: u64) -> u64 {
    ((u128::from(a) * u128::from(b)) % u128::from(modulus)) as u64
}

fn modular_power(mut base: u64, mut exponent: u64, modulus: u64) -> u64 {
    let mut result = 1_u64;
    base %= modulus;
    while exponent > 0 {
        if exponent & 1 == 1 {
            result = modular_multiply(result, base, modulus);
        }
        base = modular_multiply(base, base, modulus);
        exponent >>= 1;
    }
    result
}

/// Deterministic Miller-Rabin primality test for the full u64 domain.
/// The seven bases below are sufficient for every unsigned 64-bit integer.
#[no_mangle]
pub extern "C" fn vkaether_rust_is_prime(n: u64) -> c_int {
    if n < 2 { return 0; }
    const SMALL_PRIMES: [u64; 12] = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37];
    for prime in SMALL_PRIMES {
        if n == prime { return 1; }
        if n % prime == 0 { return 0; }
    }

    let shifts = (n - 1).trailing_zeros();
    let odd_part = (n - 1) >> shifts;
    const BASES: [u64; 7] = [2, 325, 9_375, 28_178, 450_775, 9_780_504, 1_795_265_022];

    'witness: for base in BASES {
        let base = base % n;
        if base == 0 { continue; }
        let mut value = modular_power(base, odd_part, n);
        if value == 1 || value == n - 1 { continue; }
        for _ in 1..shifts {
            value = modular_multiply(value, value, n);
            if value == n - 1 { continue 'witness; }
        }
        return 0;
    }
    1
}

/// Floating-point division for game/statistics commands. Status: 0=success, 1=null output, 2=zero divisor.
#[no_mangle]
pub extern "C" fn vkaether_rust_div_f64(a: i64, b: i64, out: *mut f64) -> c_int {
    if out.is_null() { return 1; }
    if b == 0 { return 2; }
    unsafe { out.write(a as f64 / b as f64) };
    0
}
