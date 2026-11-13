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

/// Deterministic primality predicate; returns 1 for prime, 0 otherwise.
#[no_mangle]
pub extern "C" fn vkaether_rust_is_prime(n: u64) -> c_int {
    if n < 2 { return 0; }
    if n % 2 == 0 { return i32::from(n == 2); }
    let mut divisor = 3_u64;
    while divisor <= n / divisor {
        if n % divisor == 0 { return 0; }
        divisor += 2;
    }
    1
}
