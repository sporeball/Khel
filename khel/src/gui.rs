use crate::KhelState;
use egui::{epaint::Shadow, Button, Color32, Context, Frame, Margin, Rounding};
use egui_wgpu::Renderer;
use log::info;
use wgpu::{Device, TextureFormat};
use winit::{event::WindowEvent, window::Window};

pub struct EguiRenderer {
  pub context: Context,
  pub state: egui_winit::State,
  pub renderer: Renderer,
}

impl EguiRenderer {
  pub fn new(
    device: &Device,
    output_color_format: TextureFormat,
    output_depth_format: Option<TextureFormat>,
    msaa_samples: u32,
    window: &Window,
  ) -> Self {
    let context = Context::default();
    let id = context.viewport_id();
    let state = egui_winit::State::new(context.clone(), id, &window, None, None);
    let renderer = Renderer::new(
      device,
      output_color_format,
      output_depth_format,
      msaa_samples,
    );
    Self {
      context,
      state,
      renderer,
    }
  }
  pub fn handle_input(&mut self, window: &Window, event: &WindowEvent) {
    let _ = self.state.on_window_event(window, event);
  }
}

pub fn gui(state: &mut KhelState) {
  let ctx = state.egui.context.to_owned();
  // egui::Window::new("Khel")
  egui::CentralPanel::default()
    .frame(Frame {
      outer_margin: Margin::same(10.0),
      rounding: Rounding::ZERO,
      shadow: Shadow::NONE,
      fill: Color32::TRANSPARENT,
      // stroke: Stroke::NONE,
      ..Default::default()
    })
    .show(&ctx, |ui| {
      if ui.add(Button::new("Create object")).clicked() {
        info!("creating an object...");
        let o = state.instantiate("circle_red", -1.0, 0.0);
        state.velocity(o, 100.0, 0.0);
      }
      ui.end_row();
    });
}