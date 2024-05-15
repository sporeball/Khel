use std::sync::Arc;
use image::GenericImageView;
use log::debug;
use pollster::block_on;
use wgpu::{include_wgsl, util::{BufferInitDescriptor, DeviceExt}, AddressMode, BindGroup, BindGroupDescriptor, BindGroupEntry, BindGroupLayoutDescriptor, BindGroupLayoutEntry, BindingResource, BindingType, BlendState, Buffer, BufferUsages, ColorTargetState, ColorWrites, CommandEncoderDescriptor, Device, DeviceDescriptor, Extent3d, Face, FilterMode, FragmentState, FrontFace, ImageCopyTexture, ImageDataLayout, IndexFormat, InstanceDescriptor, MultisampleState, Origin3d, PipelineCompilationOptions, PipelineLayoutDescriptor, PolygonMode, PowerPreference, PrimitiveState, PrimitiveTopology, Queue, RenderPassColorAttachment, RenderPassDescriptor, RenderPipeline, RenderPipelineDescriptor, RequestAdapterOptions, SamplerBindingType, SamplerDescriptor, ShaderStages, Surface, SurfaceConfiguration, SurfaceError, TextureAspect, TextureDescriptor, TextureDimension, TextureFormat, TextureSampleType, TextureUsages, TextureViewDescriptor, TextureViewDimension, VertexState};
use winit::{application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::ActiveEventLoop, window::{Window, WindowId}};

#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Vertex {
  position: [f32; 3],
  // color: [f32; 3],
  tex_coords: [f32; 2],
}

impl Vertex {
  fn desc() -> wgpu::VertexBufferLayout<'static> {
    wgpu::VertexBufferLayout {
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

const VERTICES: &[Vertex] = &[
  // Vertex { position: [0.0, 0.5, 0.0], color: [1.0, 0.0, 0.0] }, // A
  // Vertex { position: [-0.5, -0.5, 0.0], color: [0.0, 1.0, 0.0] }, // B
  // Vertex { position: [0.5, -0.5, 0.0], color: [0.0, 0.0, 1.0] }, // C
  Vertex { position: [-0.0625, 0.0625, 0.0], tex_coords: [0.0, 0.0]}, // top left
  Vertex { position: [-0.0625, -0.0625, 0.0], tex_coords: [0.0, 1.0]}, // bottom left
  Vertex { position: [0.0625, -0.0625, 0.0], tex_coords: [1.0, 1.0]}, // bottom right
  Vertex { position: [0.0625, 0.0625, 0.0], tex_coords: [1.0, 0.0]}, // top right
];

const INDICES: &[u16] = &[
  // 0, 1, 2,
  0, 1, 3,
  1, 2, 3
];

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
  pub vertex_buffer: Buffer,
  pub index_buffer: Buffer,
  pub num_indices: u32,
  pub diffuse_bind_group: BindGroup,
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
    let diffuse_bytes = include_bytes!("circle_red.png");
    let diffuse_image = image::load_from_memory(diffuse_bytes).unwrap();
    let diffuse_rgba = diffuse_image.to_rgba8();
    let dimensions = diffuse_image.dimensions();
    let texture_size = Extent3d {
      width: dimensions.0,
      height: dimensions.1,
      depth_or_array_layers: 1,
    };
    let diffuse_texture = device.create_texture(&TextureDescriptor {
      label: Some("diffuse_texture"),
      size: texture_size,
      mip_level_count: 1,
      sample_count: 1,
      dimension: TextureDimension::D2,
      format: TextureFormat::Rgba8UnormSrgb,
      usage: TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_DST,
      view_formats: &[],
    });
    queue.write_texture(
      // tells wgpu where to copy the pixel data
      ImageCopyTexture {
        texture: &diffuse_texture,
        mip_level: 0,
        origin: Origin3d::ZERO,
        aspect: TextureAspect::All,
      },
      // pixel data
      &diffuse_rgba,
      // texture layout
      ImageDataLayout {
        offset: 0,
        bytes_per_row: Some(4 * dimensions.0),
        rows_per_image: Some(dimensions.1),
      },
      texture_size,
    );
    // bind group
    let diffuse_texture_view = diffuse_texture.create_view(&TextureViewDescriptor::default());
    let diffuse_sampler = device.create_sampler(&SamplerDescriptor {
      address_mode_u: AddressMode::ClampToEdge,
      address_mode_v: AddressMode::ClampToEdge,
      address_mode_w: AddressMode::ClampToEdge,
      mag_filter: FilterMode::Linear,
      min_filter: FilterMode::Nearest,
      mipmap_filter: FilterMode::Nearest,
      ..Default::default()
    });
    let texture_bind_group_layout = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
      entries: &[
        BindGroupLayoutEntry {
          binding: 0,
          visibility: ShaderStages::FRAGMENT,
          ty: BindingType::Texture {
            sample_type: TextureSampleType::Float { filterable: true },
            view_dimension: TextureViewDimension::D2,
            multisampled: false,
          },
          count: None,
        },
        BindGroupLayoutEntry {
          binding: 1,
          visibility: ShaderStages::FRAGMENT,
          ty: BindingType::Sampler(SamplerBindingType::Filtering),
          count: None,
        },
      ],
      label: Some("texture_bind_group_layout"),
    });
    let diffuse_bind_group = device.create_bind_group(&BindGroupDescriptor {
      layout: &texture_bind_group_layout,
      entries: &[
        BindGroupEntry {
          binding: 0,
          resource: BindingResource::TextureView(&diffuse_texture_view),
        },
        BindGroupEntry {
          binding: 1,
          resource: BindingResource::Sampler(&diffuse_sampler),
        },
      ],
      label: Some("diffuse_bind_group"),
    });
    // shader module
    let shader = device.create_shader_module(include_wgsl!("shader.wgsl"));
    // render pipeline
    let render_pipeline_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
      label: Some("Render Pipeline Layout"),
      bind_group_layouts: &[&texture_bind_group_layout],
      push_constant_ranges: &[],
    });
    let render_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
      label: Some("Render Pipeline"),
      layout: Some(&render_pipeline_layout),
      vertex: VertexState {
        module: &shader,
        entry_point: "vs_main",
        compilation_options: PipelineCompilationOptions::default(),
        buffers: &[Vertex::desc()],
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
    // buffers
    let vertex_buffer = device.create_buffer_init(&BufferInitDescriptor {
      label: Some("Vertex Buffer"),
      contents: bytemuck::cast_slice(VERTICES),
      usage: BufferUsages::VERTEX,
    });
    let index_buffer = device.create_buffer_init(&BufferInitDescriptor {
      label: Some("Index Buffer"),
      contents: bytemuck::cast_slice(INDICES),
      usage: BufferUsages::INDEX,
    });
    let num_indices = INDICES.len() as u32;
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
      vertex_buffer,
      index_buffer,
      num_indices,
      diffuse_bind_group,
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
      render_pass.set_pipeline(&self.render_pipeline);
      render_pass.set_bind_group(0, &self.diffuse_bind_group, &[]);
      render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
      render_pass.set_index_buffer(self.index_buffer.slice(..), IndexFormat::Uint16);
      render_pass.draw_indexed(0..self.num_indices, 0, 0..1)
    }

    // submit will accept anything that implements IntoIter
    self.queue.submit(std::iter::once(encoder.finish()));
    output.present();

    Ok(())
  }
}