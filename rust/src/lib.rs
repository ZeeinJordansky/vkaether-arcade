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

#[cfg(test)]
mod tests {
    use super::*;

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
