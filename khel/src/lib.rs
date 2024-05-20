use crate::texture::DrawTexture;
use std::{mem, sync::Arc};
use cgmath::Vector3;
use log::debug;
use pollster::block_on;
use wgpu::{include_wgsl, util::{BufferInitDescriptor, DeviceExt}, BlendState, Buffer, BufferUsages, ColorTargetState, ColorWrites, CommandEncoderDescriptor, Device, DeviceDescriptor, Face, FragmentState, FrontFace, InstanceDescriptor, MultisampleState, PipelineCompilationOptions, PipelineLayoutDescriptor, PolygonMode, PowerPreference, PrimitiveState, PrimitiveTopology, Queue, RenderPassColorAttachment, RenderPassDescriptor, RenderPipeline, RenderPipelineDescriptor, RequestAdapterOptions, Surface, SurfaceConfiguration, SurfaceError, TextureUsages, TextureViewDescriptor, VertexBufferLayout, VertexState};
use winit::{application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::ActiveEventLoop, window::{Window, WindowId}};

mod texture;

#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Vertex {
  position: [f32; 3],
  // color: [f32; 3],
  tex_coords: [f32; 2],
}

impl Vertex {
  fn desc() -> VertexBufferLayout<'static> {
    VertexBufferLayout {
      array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
      step_mode: wgpu::VertexStepMode::Vertex,
      attributes: &[
        wgpu::VertexAttribute {
          offset: 0,
          shader_location: 0,
          format: wgpu::VertexFormat::Float32x3,
        },
        wgpu::VertexAttribute {
          offset: std::mem::size_of::<[f32; 3]>() as wgpu::BufferAddress,
          shader_location: 1,
          format: wgpu::VertexFormat::Float32x2,
        }
      ]
    }
  }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
struct InstanceRaw {
  model: [[f32; 4]; 4],
}

impl InstanceRaw {
  fn desc() -> VertexBufferLayout<'static> {
    VertexBufferLayout {
      array_stride: mem::size_of::<InstanceRaw>() as wgpu::BufferAddress,
      step_mode: wgpu::VertexStepMode::Instance,
      attributes: &[
        wgpu::VertexAttribute {
          offset: 0,
          shader_location: 5,
          format: wgpu::VertexFormat::Float32x4,
        },
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 4]>() as wgpu::BufferAddress,
          shader_location: 6,
          format: wgpu::VertexFormat::Float32x4,
        },
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 8]>() as wgpu::BufferAddress,
          shader_location: 7,
          format: wgpu::VertexFormat::Float32x4,
        },
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 12]>() as wgpu::BufferAddress,
          shader_location: 8,
          format: wgpu::VertexFormat::Float32x4,
        },
      ],
    }
  }
}

struct Instance {
  position: Vector3<f32>,
  // rotation: Quaternion<f32>,
}

impl Instance {
  fn to_raw(&self) -> InstanceRaw {
    InstanceRaw {
      model: (cgmath::Matrix4::from_translation(self.position)).into(),
    }
  }
}

const NUM_INSTANCES: u32 = 5;

#[derive(Default)]
pub struct App<'a> {
  pub state: Option<KhelState<'a>>,
}

impl<'a> ApplicationHandler for App<'a> {
  /// Emitted when the app has been resumed.
  fn resumed(&mut self, event_loop: &ActiveEventLoop) {
    self.state = Some(
      KhelState::new(event_loop.create_window(Window::default_attributes().with_title("Khel")).unwrap())
    );
  }
  /// Emitted when an event is received.
  fn window_event(&mut self, event_loop: &ActiveEventLoop, _id: WindowId, event: WindowEvent) {
    let Some(ref mut state) = self.state else { todo!(); };
    // state.input gets a cbance to handle the event instead
    // any events it does not handle return false
    if !state.input(&event) {
      match event {
        WindowEvent::CloseRequested => {
          event_loop.exit();
        },
        WindowEvent::RedrawRequested => {
          let Some(ref mut state) = self.state else { todo!(); };
          state.window.request_redraw();
          state.update();
          match state.render() {
            Ok(_) => {},
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
          state.diffuse_texture.vertex_buffer = texture::create_vertex_buffer(state.diffuse_texture.texture.size(), physical_size, &state.device);
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
  pub clear_color: wgpu::Color,
  pub render_pipeline: RenderPipeline,
  pub instance_buffer: Buffer,
  pub instances: Vec<Instance>,
  pub diffuse_texture: texture::Texture,
}

impl<'a> KhelState<'a> {
  /// Create a new KhelState instance.
  pub fn new(window: Window) -> KhelState<'a> {
    // basic properties
    let window = Arc::new(window);
    let size = window.inner_size();
    let clear_color = wgpu::Color::BLACK;
    // instance
    let instance = wgpu::Instance::new(InstanceDescriptor {
      backends: wgpu::Backends::PRIMARY,
      ..Default::default()
    });
    // surface
    let surface = instance.create_surface(window.clone()).unwrap();
    // adapter
    let adapter = block_on(instance.request_adapter(&RequestAdapterOptions {
      power_preference: PowerPreference::default(),
      compatible_surface: Some(&surface),
      force_fallback_adapter: false,
    })).unwrap();
    // device and queue
    let (device, queue) = block_on(adapter.request_device(
      &DeviceDescriptor {
        label: None,
        required_features: wgpu::Features::empty(),
        required_limits: wgpu::Limits::default(),
      },
      None
    )).unwrap();
    // surface configuration
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
    // texture
    let diffuse_bytes = load_binary("circle_red.png").unwrap();
    let diffuse_texture = texture::Texture::from_bytes(size, &device, &queue, &diffuse_bytes, "circle_red.png").unwrap();
    // shader module
    let shader = device.create_shader_module(include_wgsl!("shader.wgsl"));
    // render pipeline
    let render_pipeline_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
      label: Some("Render Pipeline Layout"),
      bind_group_layouts: &[&diffuse_texture.bind_group_layout],
      push_constant_ranges: &[],
    });
    let render_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
      label: Some("Render Pipeline"),
      layout: Some(&render_pipeline_layout),
      vertex: VertexState {
        module: &shader,
        entry_point: "vs_main",
        compilation_options: PipelineCompilationOptions::default(),
        buffers: &[Vertex::desc(), InstanceRaw::desc()],
      },
      fragment: Some(FragmentState {
        module: &shader,
        entry_point: "fs_main",
        compilation_options: PipelineCompilationOptions::default(),
        targets: &[Some(ColorTargetState {
          format: config.format,
          blend: Some(BlendState::ALPHA_BLENDING),
          write_mask: ColorWrites::ALL,
        })],
      }),
      primitive: PrimitiveState {
        topology: PrimitiveTopology::TriangleList,
        strip_index_format: None,
        front_face: FrontFace::Ccw,
        cull_mode: Some(Face::Back),
        polygon_mode: PolygonMode::Fill,
        unclipped_depth: false,
        conservative: false,
      },
      depth_stencil: None,
      multisample: MultisampleState {
        count: 1,
        mask: !0,
        alpha_to_coverage_enabled: false,
      },
      multiview: None,
    });
    // instance buffer
    let instances = (0..NUM_INSTANCES).map(move |x| {
      // let position = Vector3 { x: x as f32, y: 0.0, z: 0.0 } - INSTANCE_DISPLACEMENT;
      let position = Vector3 { x: x as f32 * 0.1, y: 0.0, z: 0.0 };
      Instance {
        position,
      }
    }).collect::<Vec<_>>();
    let instance_data = instances.iter().map(Instance::to_raw).collect::<Vec<_>>();
    let instance_buffer = device.create_buffer_init(&BufferInitDescriptor {
      label: Some("Instance Buffer"),
      contents: bytemuck::cast_slice(&instance_data),
      usage: BufferUsages::VERTEX,
    });
    // return value
    Self {
      window,
      surface,
      device,
      queue,
      config,
      size,
      clear_color,
      render_pipeline,
      instance_buffer,
      instances,
      diffuse_texture,
    }
  }
  /// Resize this KhelState's surface.
  pub fn resize(&mut self, new_size: PhysicalSize<u32>) {
    if new_size.width > 0 && new_size.height > 0 {
      self.size = new_size;
      self.config.width = new_size.width;
      self.config.height = new_size.height;
      self.surface.configure(&self.device, &self.config);
    }
  }
  /// Handle input.
  /// Any events not handled here will be handled in window_event on App.
  fn input(&mut self, event: &WindowEvent) -> bool {
    match event {
      WindowEvent::CursorMoved { position, .. } => {
        self.clear_color = wgpu::Color {
          r: position.x as f64 / self.size.width as f64,
          g: position.y as f64 / self.size.height as f64,
          b: 1.0,
          a: 1.0,
        };
        true
      },
      _ => false,
    }
  }
  fn update(&mut self) {
    // TODO
  }
  /// Use this KhelState to perform a render pass.
  fn render(&mut self) -> Result<(), SurfaceError> {
    let output = self.surface.get_current_texture()?;
    let view = output.texture.create_view(&TextureViewDescriptor::default());
    let mut encoder = self.device.create_command_encoder(&CommandEncoderDescriptor {
      label: Some("Render Encoder"),
    });
    {
      let mut render_pass = encoder.begin_render_pass(&RenderPassDescriptor {
        label: Some("Render Pass"),
        color_attachments: &[Some(RenderPassColorAttachment {
          view: &view,
          resolve_target: None,
          ops: wgpu::Operations {
            load: wgpu::LoadOp::Clear(self.clear_color),
            store: wgpu::StoreOp::Store,
          },
        })],
        depth_stencil_attachment: None,
        occlusion_query_set: None,
        timestamp_writes: None,
      });
      render_pass.set_vertex_buffer(1, self.instance_buffer.slice(..));
      render_pass.set_pipeline(&self.render_pipeline);
      render_pass.draw_texture_instanced(&self.diffuse_texture, 0..self.instances.len() as u32);
    }

    // submit will accept anything that implements IntoIter
    self.queue.submit(std::iter::once(encoder.finish()));
    output.present();

    Ok(())
  }
}

pub fn load_binary(filename: &str) -> anyhow::Result<Vec<u8>> {
  let path = std::path::Path::new(env!("OUT_DIR"))
    .join("assets")
    .join(filename);
  let data = std::fs::read(path)?;
  Ok(data)
}