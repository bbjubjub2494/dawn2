// Copyright (C) 2017-2018 Baidu, Inc. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of Baidu, Inc., nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#![crate_name = "helloworldsampleenclave"]
#![crate_type = "staticlib"]

#![cfg_attr(not(target_env = "sgx"), no_std)]
#![cfg_attr(target_env = "sgx", feature(rustc_private))]

extern crate sgx_types;
#[cfg(not(target_env = "sgx"))]
#[macro_use]
extern crate sgx_tstd as std;
extern crate sgx_tunittest;

#[cfg(target_env = "sgx")]
extern crate core;

use sgx_types::*;
use std::string::String;
use std::vec::Vec;
use std::io::{self, Write};
use std::slice;
use sgx_tunittest::*;

extern crate serde_cbor;
extern crate serde;
#[macro_use]
extern crate serde_derive;

mod tests;

#[no_mangle]
pub extern "C" fn say_something(some_string: *const u8, some_len: usize) -> sgx_status_t {

    let str_slice = unsafe { slice::from_raw_parts(some_string, some_len) };
    let _ = io::stdout().write(str_slice);

    // A sample &'static string
    let rust_raw_string = "This is a in-Enclave ";
    // An array
    let word:[u8;4] = [82, 117, 115, 116];
    // An vector
    let word_vec:Vec<u8> = vec![32, 115, 116, 114, 105, 110, 103, 33];

    // Construct a string from &'static string
    let mut hello_string = String::from(rust_raw_string);

    // Iterate on word array
    for c in word.iter() {
        hello_string.push(*c as char);
    }

    // Rust style convertion
    hello_string += String::from_utf8(word_vec).expect("Invalid UTF-8")
                                               .as_str();

    // Ocall to normal world for output
    println!("{}", &hello_string);

    rsgx_unit_tests!(
tests::bennofs::test,
tests::bennofs::std_tests::test,
tests::canonical::std_tests::integer_canonical_sort_order,
tests::canonical::std_tests::string_canonical_sort_order,
tests::canonical::std_tests::bytes_canonical_sort_order,
tests::canonical::std_tests::simple_data_canonical_sort_order,
tests::canonical::std_tests::major_type_canonical_sort_order,
tests::canonical::std_tests::test_rfc_example,
tests::de::test_str,
tests::de::test_bytes,
tests::de::test_int,
tests::de::test_float,
tests::de::test_indefinite_object,
tests::de::std_tests::test_string1,
tests::de::std_tests::test_string2,
tests::de::std_tests::test_string3,
tests::de::std_tests::test_byte_string,
tests::de::std_tests::test_numbers1,
tests::de::std_tests::test_numbers2,
tests::de::std_tests::test_numbers3,
tests::de::std_tests::test_bool,
tests::de::std_tests::test_trailing_bytes,
tests::de::std_tests::test_list1,
tests::de::std_tests::test_list2,
tests::de::std_tests::test_object,
tests::de::std_tests::test_indefinite_object,
tests::de::std_tests::test_indefinite_list,
tests::de::std_tests::test_indefinite_string,
tests::de::std_tests::test_indefinite_byte_string,
tests::de::std_tests::test_multiple_indefinite_strings,
tests::de::std_tests::test_float,
tests::de::std_tests::test_self_describing,
tests::de::std_tests::test_f16,
tests::de::std_tests::test_crazy_list,
tests::de::std_tests::test_nan,
tests::de::std_tests::test_32f16,
tests::de::std_tests::test_kietaub_file,
tests::de::std_tests::test_option_roundtrip,
tests::de::std_tests::test_option_none_roundtrip,
tests::de::std_tests::test_variable_length_map,
tests::de::std_tests::test_object_determinism_roundtrip,
tests::de::std_tests::test_slice_offset,
tests::de::std_tests::stream_deserializer,
tests::de::std_tests::stream_deserializer_eof,
tests::de::std_tests::stream_deserializer_eof_in_indefinite,
tests::de::std_tests::crash,
tests::r#enum::std_tests::test_enum,
tests::r#enum::std_tests::test_repr_enum,
tests::r#enum::std_tests::test_data_enum,
tests::r#enum::std_tests::test_serialize,
tests::r#enum::std_tests::test_newtype_struct,
tests::r#enum::std_tests::test_variable_length_array,
tests::r#enum::std_tests::test_enum_as_map,
tests::r#enum::test_simple_data_enum_roundtrip,
tests::ser::test_str,
tests::ser::test_list,
tests::ser::test_float,
tests::ser::test_integer,
tests::ser::std_tests::test_string,
tests::ser::std_tests::test_list,
tests::ser::std_tests::test_object,
tests::ser::std_tests::test_object_list_keys,
tests::ser::std_tests::test_object_object_keys,
tests::ser::std_tests::test_float,
tests::ser::std_tests::test_f32,
tests::ser::std_tests::test_infinity,
tests::ser::std_tests::test_neg_infinity,
tests::ser::std_tests::test_nan,
tests::ser::std_tests::test_integer,
tests::ser::std_tests::test_self_describing,
tests::ser::std_tests::test_ip_addr,
tests::ser::std_tests::test_byte_string,
tests::ser::std_tests::test_half,
tests::std_types::std_tests::test_bool_false,
tests::std_types::std_tests::test_bool_true,
tests::std_types::std_tests::test_isize_neg_256,
tests::std_types::std_tests::test_isize_neg_257,
tests::std_types::std_tests::test_isize_255,
tests::std_types::std_tests::test_i8_5,
tests::std_types::std_tests::test_i8_23,
tests::std_types::std_tests::test_i8_24,
tests::std_types::std_tests::test_i8_neg_128,
tests::std_types::std_tests::test_u32_98745874,
tests::std_types::std_tests::test_f32_1234_point_5,
tests::std_types::std_tests::test_f64_12345_point_6,
tests::std_types::std_tests::test_f64_nan,
tests::std_types::std_tests::test_f64_infinity,
tests::std_types::std_tests::test_f64_neg_infinity,
tests::std_types::std_tests::test_char_null,
tests::std_types::std_tests::test_char_broken_heart,
tests::std_types::std_tests::test_str_pangram_de,
tests::std_types::std_tests::test_unit,
tests::std_types::std_tests::test_unit_struct,
tests::std_types::std_tests::test_newtype_struct,
tests::std_types::std_tests::test_option_none,
tests::std_types::std_tests::test_option_some,
tests::std_types::std_tests::test_person_struct,
tests::std_types::std_tests::test_optional_person_struct,
tests::std_types::std_tests::test_color_enum,
tests::std_types::std_tests::test_color_enum_transparent,
tests::std_types::std_tests::test_color_enum_with_alpha,
tests::std_types::std_tests::test_i128_a,
tests::std_types::std_tests::test_i128_b,
tests::std_types::std_tests::test_u128,
tests::value::std_tests::serde,
tests::de::std_tests::test_deserializer_enums,
tests::de::std_tests::test_packed_deserialization,
tests::de::std_tests::test_ipaddr_deserialization,
);

    sgx_status_t::SGX_SUCCESS
}
