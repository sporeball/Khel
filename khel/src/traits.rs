// use crate::chart::BpmList;
use wgpu::Extent3d;
use winit::dpi::PhysicalSize;

/// A trait for determining where on the keyboard a char is located.
pub trait KeyboardPosition {
  fn column(&self) -> Option<u8>;
  fn row(&self) -> Option<u8>;
}

impl KeyboardPosition for char {
  /// Return the column on the keyboard that a char is in.
  fn column(&self) -> Option<u8> {
    match self {
      'q' | 'a' | 'z' => Some(0),
      'w' | 's' | 'x' => Some(1),
      'e' | 'd' | 'c' => Some(2),
      'r' | 'f' | 'v' => Some(3),
      't' | 'g' | 'b' => Some(4),
      'y' | 'h' | 'n' => Some(5),
      'u' | 'j' | 'm' => Some(6),
      'i' | 'k' | ',' => Some(7),
      'o' | 'l' | '.' => Some(8),
      'p' | ';' | '/' => Some(9),
      _ => None,
    }
  }
  /// Return the row on the keyboard that a char is in.
  fn row(&self) -> Option<u8> {
    match self {
      'q' | 'w' | 'e' | 'r' | 't' | 'y' | 'u' | 'i' | 'o' | 'p' => Some(1),
      'a' | 's' | 'd' | 'f' | 'g' | 'h' | 'j' | 'k' | 'l' | ';' => Some(2),
      'z' | 'x' | 'c' | 'v' | 'b' | 'n' | 'm' | ',' | '.' | '/' => Some(4),
      _ => None,
    }
  }
}

pub trait ZeroToTwo {
  fn zero_to_two(&self, window_size: PhysicalSize<u32>) -> (f32, f32);
}

impl ZeroToTwo for Extent3d {
  fn zero_to_two(&self, window_size: PhysicalSize<u32>) -> (f32, f32) {
    let x_pixel = 2.0 / window_size.width as f32;
    let y_pixel = 2.0 / window_size.height as f32;
    let width = self.width as f32 * x_pixel;
    let height = self.height as f32 * y_pixel;
    (width, height)
  }
}

// impl<P> ZeroToTwo for PhysicalSize<P> {
//   fn zero_to_two(&self, window_size: PhysicalSize<P>) {
//     let x_pixel = 2.0 / window_size.width as f32;
//     let y_pixel = 2.0 / window_size.height as f32;
//     let width = self.width as f32 * x_pixel;
//     let height = self.height as f32 * y_pixel;
//     (width, height)
//   }
// }
