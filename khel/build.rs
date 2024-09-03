use std::env;
use anyhow::*;
use fs_extra::{copy_items, dir::CopyOptions};

fn main() -> Result<(), anyhow::Error> {
  // println!("cargo:rerun-if-changed=build.rs");
  println!("cargo:rerun-if-changed=assets/*");

  let out_dir = env::var("OUT_DIR")?;
  let mut copy_options = CopyOptions::new();
  copy_options.overwrite = true;
  let paths_to_copy = vec!["assets/"];
  copy_items(&paths_to_copy, out_dir, &copy_options)?;

  Ok(())
}
