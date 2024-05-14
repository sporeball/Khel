use std::sync::Arc;
use pollster::block_on;
use wgpu::{CommandEncoderDescriptor, Device, DeviceDescriptor, InstanceDescriptor, PowerPreference, Queue, RenderPassColorAttachment, RenderPassDescriptor, RequestAdapterOptions, Surface, SurfaceConfiguration, SurfaceError, TextureUsages, TextureViewDescriptor};
use winit::{application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::ActiveEventLoop, window::{Window, WindowId}};

#[derive(Default)]
pub struct App<'a> {
  pub state: Option<KhelState<'a>>,
}

impl<'a> ApplicationHandler for App<'a> {
  fn resumed(&mut self, event_loop: &ActiveEventLoop) {
    self.state = Some(
      KhelState::new(event_loop.create_window(Window::default_attributes().with_title("Khel")).unwrap())
    );
  }
  fn window_event(&mut self, event_loop: &ActiveEventLoop, _id: WindowId, event: WindowEvent) {
    let Some(ref mut state) = self.state else { todo!(); };
    if !state.input(&event) {
      match event {
        WindowEvent::CloseRequested => {
          event_loop.exit();
        },
        WindowEvent::RedrawRequested => {
          let Some(ref mut state) = self.state else { todo!(); };
          // state.window.request_redraw();
          state.update();
          match state.render() {
            Ok(_) => {}
            // reconfigure the surface if lost
            Err(wgpu::SurfaceError::Lost) => state.resize(state.size),
            // the system is out of memory, we should probably quit
            Err(wgpu::SurfaceError::OutOfMemory) => event_loop.exit(),
            // all other errors (Outdated, Timeout) should be resolved by the next frame
            Err(e) => eprintln!("{:?}", e),
          }
        },
        WindowEvent::Resized(physical_size) => {
          let Some(ref mut state) = self.state else { todo!(); };
          state.resize(physical_size);
        },
        WindowEvent::ScaleFactorChanged { scale_factor: _, inner_size_writer: _ } => todo!(),
        _ => (),
      }
    }
  }
}

pub struct KhelState<'a> {
  pub window: Arc<Window>,
  pub surface: Surface<'a>,
  pub device: Device,
  pub queue: Queue,
  pub config: SurfaceConfiguration,
  pub size: winit::dpi::PhysicalSize<u32>,
}

impl<'a> KhelState<'a> {
  pub fn new(window: Window) -> KhelState<'a> {
    let window = Arc::new(window);
    let size = window.inner_size();
    let instance = wgpu::Instance::new(InstanceDescriptor {
      backends: wgpu::Backends::PRIMARY,
      ..Default::default()
    });
    let surface = instance.create_surface(window.clone()).unwrap();
    let adapter = block_on(instance.request_adapter(&RequestAdapterOptions {
      power_preference: PowerPreference::default(),
      compatible_surface: Some(&surface),
      force_fallback_adapter: false,
    })).unwrap();
    let (device, queue) = block_on(adapter.request_device(
      &DeviceDescriptor {
        label: None,
        required_features: wgpu::Features::empty(),
        required_limits: wgpu::Limits::default(),
      },
      None
    )).unwrap();
    let surface_capabilities = surface.get_capabilities(&adapter);
    // let surface_format = surface_capabilities.formats.iter()
    //   .copied()
    //   .filter(|f| f.is_srgb())
    //   .next()
    //   .unwrap_or(surface_capabilities.formats[0]);
    let config = SurfaceConfiguration {
      usage: TextureUsages::RENDER_ATTACHMENT,
      format: surface_capabilities.formats[0],
      width: size.width,
      height: size.height,
      present_mode: surface_capabilities.present_modes[0],
      desired_maximum_frame_latency: 2,
      alpha_mode: surface_capabilities.alpha_modes[0],
      view_formats: Vec::new(),
    };
    surface.configure(&device, &config);
    Self {
      window,
      surface,
      device,
      queue,
      config,
      size,
    }
  }
  pub fn resize(&mut self, new_size: PhysicalSize<u32>) {
    if new_size.width > 0 && new_size.height > 0 {
      self.size = new_size;
      self.config.width = new_size.width;
      self.config.height = new_size.height;
      self.surface.configure(&self.device, &self.config);
    }
  }
  fn input(&mut self, _event: &WindowEvent) -> bool {
    // TODO
    false
  }
  fn update(&mut self) {
    // TODO
  }
  fn render(&mut self) -> Result<(), SurfaceError> {
    let output = self.surface.get_current_texture()?;
    let view = output.texture.create_view(&TextureViewDescriptor::default());
    let mut encoder = self.device.create_command_encoder(&CommandEncoderDescriptor {
      label: Some("Render Encoder"),
    });
    {
      let _render_pass = encoder.begin_render_pass(&RenderPassDescriptor {
        label: Some("Render Pass"),
        color_attachments: &[Some(RenderPassColorAttachment {
          view: &view,
          resolve_target: None,
          ops: wgpu::Operations {
            load: wgpu::LoadOp::Clear(wgpu::Color {
                r: 0.1,
                g: 0.2,
                b: 0.3,
                a: 1.0,
            }),
            store: wgpu::StoreOp::Store,
          },
        })],
        depth_stencil_attachment: None,
        occlusion_query_set: None,
        timestamp_writes: None,
      });
    }

    // submit will accept anything that implements IntoIter
    self.queue.submit(std::iter::once(encoder.finish()));
    output.present();

    Ok(())
  }
}