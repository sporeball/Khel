use wgpu::Extent3d;
use winit::dpi::PhysicalSize;

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
