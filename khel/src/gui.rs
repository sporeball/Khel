use egui::{epaint::Shadow, Button, Color32, Context, Frame, Margin, Rounding};
use egui_wgpu::{Renderer, ScreenDescriptor};
use log::info;
use wgpu::{CommandEncoder, Device, Queue, RenderPassColorAttachment, RenderPassDescriptor, TextureFormat, TextureView};
use winit::{event::WindowEvent, window::Window};

pub struct EguiRenderer {
  pub context: Context,
  state: egui_winit::State,
  renderer: Renderer,
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
    // let visuals = Visuals {
    //   window_rounding: Rounding::ZERO,
    //   window_shadow: Shadow::NONE,
    //   window_fill: Color32::TRANSPARENT,
    //   // window_stroke: Stroke::NONE,
    //   ..Default::default()
    // };
    // context.set_visuals(visuals);
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
  pub fn draw(
    &mut self,
    device: &Device,
    queue: &Queue,
    encoder: &mut CommandEncoder,
    window: &Window,
    window_surface_view: &TextureView,
    screen_descriptor: ScreenDescriptor,
    run_ui: impl FnOnce(&Context),
  ) {
    let raw_input = self.state.take_egui_input(window);
    let full_output = self.context.run(raw_input, |ui| {
      run_ui(&self.context);
    });
    self.state.handle_platform_output(&window, full_output.platform_output);
    let tris = self.context
      .tessellate(full_output.shapes, full_output.pixels_per_point);
    for (id, image_delta) in &full_output.textures_delta.set {
      self.renderer.update_texture(&device, &queue, *id, &image_delta);
    }
    self.renderer.update_buffers(&device, &queue, encoder, &tris, &screen_descriptor);
    let mut rpass = encoder.begin_render_pass(&RenderPassDescriptor {
      color_attachments: &[Some(RenderPassColorAttachment {
        view: &window_surface_view,
        resolve_target: None,
        ops: wgpu::Operations {
          load: wgpu::LoadOp::Load,
          store: wgpu::StoreOp::Store,
        },
      })],
      depth_stencil_attachment: None,
      label: Some("egui main render pass"),
      timestamp_writes: None,
      occlusion_query_set: None,
    });
    self.renderer.render(&mut rpass, &tris, &screen_descriptor);
    drop(rpass);
    for x in &full_output.textures_delta.free {
      self.renderer.free_texture(x)
    }
  }
}

pub fn gui(ui: &Context) {
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
    .show(&ui, |ui| {
      if ui.add(Button::new("Click me")).clicked() {
        info!("button pressed!")
      }
      ui.end_row();
    });
}