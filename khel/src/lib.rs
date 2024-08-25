use crate::{chart::{ChartInfo, TimingInfo}, object::{DrawObject, Object, Objects}};
use std::fs::File;
use std::io::{self, BufRead};
use std::mem;
use std::path::Path;
use std::sync::Arc;
use std::time::Duration;
// use egui::Context;
use egui_wgpu::ScreenDescriptor;
use fps_ticker::Fps;
use gui::EguiRenderer;
use sound::Sound;
use cgmath::{Vector2, Vector3};
use log::info;
use pollster::block_on;
use wgpu::{include_wgsl, BlendState, ColorTargetState, ColorWrites, CommandEncoder, CommandEncoderDescriptor, Device, DeviceDescriptor, Face, FragmentState, FrontFace, InstanceDescriptor, MultisampleState, PipelineLayoutDescriptor, PolygonMode, PowerPreference, PrimitiveState, PrimitiveTopology, Queue, RenderPassColorAttachment, RenderPassDescriptor, RenderPipeline, RenderPipelineDescriptor, RequestAdapterOptions, Surface, SurfaceConfiguration, SurfaceError, TextureUsages, TextureView, TextureViewDescriptor, VertexBufferLayout, VertexState};
// use winit::{application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::ActiveEventLoop, window::{Window, WindowId}};
use winit::{dpi::PhysicalSize, event::{ElementState, KeyEvent, WindowEvent}, window::Window};

pub mod chart;
pub mod gui;
pub mod object;
pub mod sound;
pub mod texture;
pub mod traits;

#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Vertex {
  position: [f32; 3],
  tex_coords: [f32; 2],
}

impl Vertex {
  fn desc() -> VertexBufferLayout<'static> {
    VertexBufferLayout {
      array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
      step_mode: wgpu::VertexStepMode::Vertex,
      attributes: &[
        // position
        wgpu::VertexAttribute {
          offset: 0,
          shader_location: 0,
          format: wgpu::VertexFormat::Float32x3,
        },
        // tex_coords
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
/// A raw object instance to send to the shader.
struct InstanceRaw {
  // mat4
  model: [[f32; 4]; 4],
}

impl InstanceRaw {
  fn desc() -> VertexBufferLayout<'static> {
    VertexBufferLayout {
      array_stride: mem::size_of::<InstanceRaw>() as wgpu::BufferAddress,
      step_mode: wgpu::VertexStepMode::Instance,
      attributes: &[
        // model vec4 #1
        wgpu::VertexAttribute {
          offset: 0,
          shader_location: 5,
          format: wgpu::VertexFormat::Float32x4,
        },
        // model vec4 #2
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 4]>() as wgpu::BufferAddress,
          shader_location: 6,
          format: wgpu::VertexFormat::Float32x4,
        },
        // model vec4 #3
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 8]>() as wgpu::BufferAddress,
          shader_location: 7,
          format: wgpu::VertexFormat::Float32x4,
        },
        // model vec4 #4
        wgpu::VertexAttribute {
          offset: mem::size_of::<[f32; 12]>() as wgpu::BufferAddress,
          shader_location: 8,
          format: wgpu::VertexFormat::Float32x4,
        },
      ],
    }
  }
}

/// An object instance.
#[derive(Clone)]
pub struct Instance {
  // t: String,
  position: Vector3<f32>,
  velocity: Vector2<f32>,
  // create_time: Duration,
  // destroy_time: Duration,
  // rotation: Quaternion<f32>,
}

impl Instance {
  /// Convert this Instance to an InstanceRaw to be sent to the shader.
  fn to_raw(&self) -> InstanceRaw {
    InstanceRaw {
      model: (cgmath::Matrix4::from_translation(self.position)).into(),
    }
  }
}

// #[derive(Default)]
// pub struct App<'a> {
//   pub state: Option<KhelState<'a>>,
// }

// impl<'a> ApplicationHandler for App<'a> {
//   /// Emitted when the app has been resumed.
//   fn resumed(&mut self, event_loop: &ActiveEventLoop) {
//     self.state = Some(
//       KhelState::new(event_loop.create_window(Window::default_attributes().with_title("Khel")).unwrap())
//     );
//     let Some(ref mut state) = self.state else { todo!(); };
//     let device = &state.device;
//     // objects
//     let objects = &mut state.objects;
//     let [
//       circle_red,
//       circle_green,
//     ] = objects.as_mut_slice() else { todo!(); };
//     circle_red.instantiate(0.9, 0.9, device);
//     circle_green.instantiate(0.0, 0.0, device);
//     // sounds
//     // let sounds = &mut state.sounds;
//     // let [
//     //   sound,
//     // ] = sounds.as_mut_slice() else { todo!(); };
//     // sound.play();
//   }
  /// Emitted when an event is received.
//   fn window_event(&mut self, event_loop: &ActiveEventLoop, _id: WindowId, event: WindowEvent) {
//     let Some(ref mut state) = self.state else { todo!(); };
//     // state.input gets a cbance to handle the event instead
//     // any events it does not handle return false
//     if !state.input(&event) {
//       match event {
//         WindowEvent::CloseRequested => {
//           event_loop.exit();
//         },
//         WindowEvent::RedrawRequested => {
//           let Some(ref mut state) = self.state else { todo!(); };
//           state.window.request_redraw();
//           state.update();
//           match state.render() {
//             Ok(_) => {},
//             // reconfigure the surface if lost
//             Err(wgpu::SurfaceError::Lost) => state.resize(state.size),
//             // the system is out of memory, we should probably quit
//             Err(wgpu::SurfaceError::OutOfMemory) => event_loop.exit(),
//             // all other errors (Outdated, Timeout) should be resolved by the next frame
//             Err(e) => eprintln!("{:?}", e),
//           }
//         },
//         WindowEvent::Resized(physical_size) => {
//           let Some(ref mut state) = self.state else { todo!(); };
//           state.resize(physical_size);
//           // state.diffuse_texture.vertex_buffer = texture::create_vertex_buffer(state.diffuse_texture.texture.size(), physical_size, &state.device);
//           for object in &mut state.objects {
//             object.texture.vertex_buffer = texture::create_vertex_buffer(object.texture.texture.size(), physical_size, &state.device);
//           }
//         },
//         WindowEvent::ScaleFactorChanged { scale_factor: _, inner_size_writer: _ } => todo!(),
//         _ => (),
//       }
//     }
//   }
// }

pub struct KhelState<'a> {
  pub window: Arc<Window>,
  pub surface: Surface<'a>,
  pub device: Device,
  pub queue: Queue,
  pub config: SurfaceConfiguration,
  pub size: winit::dpi::PhysicalSize<u32>,
  pub clear_color: wgpu::Color,
  pub render_pipeline: RenderPipeline,
  pub fps: Fps,
  pub time: Duration,
  // pub objects: HashMap<String, Object>,
  pub objects: Objects,
  pub min_available_object_id: u32,
  pub sounds: Vec<Sound>,
  pub egui: EguiRenderer,
  pub chart_path: String,
  pub chart_info: ChartInfo,
  pub timing_info: Option<Vec<TimingInfo>>,
  pub xmod: f32,
  pub ratemod: f32,
  pub prev_ho_id: Option<u32>,
}

impl<'a> KhelState<'a> {
  /// Create a new KhelState instance.
  pub fn new(window: Arc<Window>, gl: bool) -> KhelState<'a> {
    // basic properties
    // let window = Arc::new(window);
    let size = window.inner_size();
    let clear_color = wgpu::Color::BLACK;
    // instance
    let backends = match gl {
      true => wgpu::Backends::SECONDARY,
      false => wgpu::Backends::PRIMARY,
    };
    let instance = wgpu::Instance::new(InstanceDescriptor {
      backends,
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
        required_limits: wgpu::Limits::downlevel_defaults(),
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
    // objects!
    // let objects: HashMap<String, Object> = HashMap::new();
    let objects = Objects::default();
    let min_available_object_id = 0;
    // shader module
    let shader = device.create_shader_module(include_wgsl!("shader.wgsl"));
    // render pipeline
    let render_pipeline_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
      label: Some("Render Pipeline Layout"),
      bind_group_layouts: &[&device.create_bind_group_layout(&texture::bgl_desc())],
      push_constant_ranges: &[],
    });
    let render_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
      label: Some("Render Pipeline"),
      layout: Some(&render_pipeline_layout),
      vertex: VertexState {
        module: &shader,
        entry_point: "vs_main",
        // compilation_options: PipelineCompilationOptions::default(),
        buffers: &[Vertex::desc(), InstanceRaw::desc()],
      },
      fragment: Some(FragmentState {
        module: &shader,
        entry_point: "fs_main",
        // compilation_options: PipelineCompilationOptions::default(),
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
    let fps = Fps::default();
    let time = Duration::ZERO;
    // sounds
    // let sound = Sound::new("sound.wav");
    let sounds = vec![
    //   sound,
    ];
    // egui
    let egui = EguiRenderer::new(
      &device,
      config.format,
      None,
      1,
      &window
    );
    let chart_path = String::new();
    // info
    let chart_info = ChartInfo::none();
    let timing_info = None;
    // speed mods
    let xmod = 4.0;
    let ratemod = 1.0;
    // TODO: remove this field and find a more elegant solution
    let prev_ho_id: Option<u32> = None;
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
      fps,
      time,
      objects,
      min_available_object_id,
      sounds,
      egui,
      chart_path,
      chart_info,
      timing_info,
      xmod,
      ratemod,
      prev_ho_id,
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
  pub fn input(&mut self, event: &WindowEvent) -> bool {
    self.window.request_redraw();
    // match event {
    //   WindowEvent::CursorMoved { position, .. } => {
    //     self.clear_color = wgpu::Color {
    //       r: position.x as f64 / self.size.width as f64,
    //       g: position.y as f64 / self.size.height as f64,
    //       b: 1.0,
    //       a: 1.0,
    //     };
    //     true
    //   },
    //   _ => false,
    // }
    match event {
      WindowEvent::CloseRequested => {
        return false;
      },
      WindowEvent::KeyboardInput { device_id: _, event, is_synthetic: _ } => {
        let KeyEvent {
          physical_key: _,
          logical_key,
          text: _,
          location: _,
          state,
          repeat: _,
          ..
        } = event;
        match state {
          ElementState::Pressed => {
            // if let winit::keyboard::Key::Character(c) = logical_key {
            //   info!("the {} key was pressed", c);
            // }
          },
          ElementState::Released => {},
        };
      },
      WindowEvent::RedrawRequested => {
        self.window.request_redraw();
        // self.update();
        match self.render() {
          Ok(_) => {},
          // reconfigure the surface if lost
          Err(wgpu::SurfaceError::Lost) => self.resize(self.size),
          // the system is out of memory, we should probably quit
          Err(wgpu::SurfaceError::OutOfMemory) => return false,
          // all other errors (Outdated, Timeout) should be resolved by the next frame
          Err(e) => eprintln!("{:?}", e),
        }
      },
      WindowEvent::Resized(physical_size) => {
        self.resize(*physical_size);
        for object in self.objects.map.values_mut() {
          object.texture.vertex_buffer = texture::create_vertex_buffer(object.texture.texture.size(), *physical_size, &self.device);
        }
      },
      // WindowEvent::ScaleFactorChanged { scale_factor: _, inner_size_writer: _ } => todo!(),
      _ => (),
    }
    // egui
    self.egui.handle_input(&mut self.window, &event);
    // return value
    true
  }
  pub fn update(&mut self) {
    // move objects
    for object in self.objects.map.values_mut() {
      for instance in &mut object.instances.values_mut() {
        // window coordinates are [-1.0, 1.0], so we have to multiply by 2
        instance.position.x += instance.velocity.x / self.size.width as f32 / 1000.0 * 2.0;
        instance.position.y += instance.velocity.y / self.size.height as f32 / 1000.0 * 2.0;
      }
    }
    // try to play chart
    let chart_info = &mut self.chart_info;
    let chart = &chart_info.chart;
    // let ticks = &chart.ticks.0;
    // TODO: is this expensive enough to avoid?
    let ticks = chart.ticks.0.clone();
    let instance_tick_u32 = chart_info.instance_tick;
    let hit_tick_u32 = chart_info.hit_tick;
    let end_tick_u32 = chart_info.end_tick;
    // start to instantiate objects
    // let mut prev_ho_id: Option<u32> = None;
    let Some(ref timing_info) = self.timing_info else { return; };
    let Some(instance_tick) = ticks.get(instance_tick_u32 as usize) else { return; };
    let Some(instance_tick_timing_info) = &timing_info.get(instance_tick_u32 as usize) else { unreachable!(); };
    // TODO: stop repeated calls
    if self.time > chart_info.music_time {
      chart.audio.play();
    }
    if self.time > instance_tick_timing_info.instance_time {
      // durations
      let bpm = chart.metadata.bpms.at_tick(instance_tick_u32).value * self.ratemod as f64;
      let one_minute = Duration::from_secs(60);
      let one_beat = one_minute.div_f64(bpm);
      // calculate travel time
      let (_, ho_height) = zero_to_two(32, 32, self.size);
      let instance_y = match instance_tick_u32 {
        0 => -1.0,
        _ => {
          // previous hit object's current y coordinate minus the distance from the previous tick
          // to the current one
          let prev_ho_id = self.prev_ho_id.unwrap();
          let prev_ho_instance = self.get_instance(prev_ho_id);
          let prev_ho_y = prev_ho_instance.position.y;
          let prev_tick = &ticks[instance_tick_u32 as usize - 1];
          prev_ho_y - prev_tick.distance(ho_height, self.chart_info.chart.metadata.divisors.at_tick(instance_tick_u32 - 1).value, self.xmod)
        },
      };
      let heights_to_travel = (0.0 - instance_y) / ho_height;
      // 1/4 = 1 height to travel
      let travel_time = one_beat.mul_f32(heights_to_travel).div_f32(self.xmod).as_secs_f32();
      // let travel_distance = self.size.height as f32 * 0.5;
      let travel_distance = self.size.height as f32 * ((0.0 - instance_y) / 2.0);
      let yv = travel_distance / travel_time;
      // instantiate timing line
      let line = self.instantiate(
        instance_tick.timing_line_asset(
          self.chart_info.chart.metadata.divisors.at_tick(instance_tick_u32).value,
          self.chart_info.chart.metadata.divisors.at_tick(instance_tick_u32).units_elapsed
        ),
        -1.0,
        instance_y
      );
      self.velocity(line, 0.0, yv);
      // instantiate hit objects
      for hit_object in &instance_tick.hit_objects.0 {
        let o = self.instantiate(hit_object.asset(), hit_object.lane_x(), instance_y);
        self.velocity(o, 0.0, yv);
        // we can set prev_ho_id here even in the presence of multiple hit objects because they
        // should be synced
        self.prev_ho_id = Some(o);
      }
      // move to the next tick
      self.chart_info.instance_tick += 1;
      self.chart_info.chart.metadata.divisors.at_tick_mut(instance_tick_u32).units_elapsed += (instance_tick.length + 1) as u32;
    }
    let Some(ref timing_info) = self.timing_info else { return; };
    let Some(hit_tick) = ticks.get(hit_tick_u32 as usize) else { return; };
    let Some(hit_tick_timing_info) = &timing_info.get(hit_tick_u32 as usize) else { unreachable!(); };
    if self.time > hit_tick_timing_info.hit_time {
      self.chart_info.hit_tick += 1;
    }
  }
  /// Use this KhelState to perform a render pass.
  pub fn render(&mut self) -> Result<(), SurfaceError> {
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
      for object in self.objects.map.values_mut() {
        object.instance_buffer = object::create_instance_buffer(&object.instances, &self.device);
      }
      for object in self.objects.map.values() {
        render_pass.draw_object_instanced(object);
      }
    }

    // self.egui.draw(
    //   &self.device,
    //   &self.queue,
    //   &mut encoder,
    //   &self.window,
    //   &view,
    //   // screen_descriptor,
    //   |ctx| gui::gui(ctx),
    // );
    self.render_gui(
      &mut encoder,
      &view,
      // TODO: pass one of several
      gui::gui,
    );

    // submit will accept anything that implements IntoIter
    self.queue.submit(std::iter::once(encoder.finish()));
    output.present();

    Ok(())
  }
  pub fn render_gui(
    &mut self,
    encoder: &mut CommandEncoder,
    window_surface_view: &TextureView,
    // run_ui: impl FnOnce(&mut KhelState),
    run_ui: fn(&mut KhelState),
  ) {
    let screen_descriptor = ScreenDescriptor {
      size_in_pixels: [self.config.width, self.config.height],
      pixels_per_point: self.window.scale_factor() as f32,
    };
    let raw_input = self.egui.state.take_egui_input(&self.window);
    // let full_output = self.egui.context.run(raw_input, |_ui| {
    //   // run_ui(&self.egui.context);
    // });
    self.egui.context.begin_frame(raw_input);
    run_ui(self);
    let full_output = self.egui.context.end_frame();
    self.egui.state.handle_platform_output(&self.window, full_output.platform_output);
    let tris = self.egui.context
      .tessellate(full_output.shapes, full_output.pixels_per_point);
    for (id, image_delta) in &full_output.textures_delta.set {
      self.egui.renderer.update_texture(&self.device, &self.queue, *id, &image_delta);
    }
    self.egui.renderer.update_buffers(&self.device, &self.queue, encoder, &tris, &screen_descriptor);
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
    self.egui.renderer.render(&mut rpass, &tris, &screen_descriptor);
    // cleanup
    drop(rpass);
    for x in &full_output.textures_delta.free {
      self.egui.renderer.free_texture(x)
    }
  }
  /// Instantiate an object at the given coordinates.
  /// Returns the ID of the object instance.
  pub fn instantiate(&mut self, t: &str, x: f32, y: f32) -> u32 {
    // create an entry in objects if none exists
    if self.objects.map.get(t).is_none() {
      let filename = format!("{}.png", t);
      let object = Object::from_file(
        filename.as_str(),
        self.window.inner_size(),
        &self.device,
        &self.queue
      ).unwrap();
      self.objects.map.insert(t.to_string(), object);
    }
    // create an instance
    let object = self.objects.map.get_mut(t).unwrap();
    let id = self.min_available_object_id;
    let instance = Instance {
      // t: t.to_string(),
      position: Vector3 { x, y, z: 0.0 },
      velocity: Vector2 { x: 0.0, y: 0.0 },
      // create_time: self.time,
      // destroy_time: Duration::MAX,
    };
    // push the instance
    object.instances.insert(id, instance.clone());
    object.instance_buffer = object::create_instance_buffer(&object.instances, &self.device);
    // info!("created {} instance (id: {})", t, id);
    self.min_available_object_id += 1;
    id
  }
  /// Instantiate an object at the bottom of the given lane.
  /// Returns the ID of the object instance.
  // pub fn instantiate_in_lane(&mut self, lane: u8, t: &str) -> u32 {
  //   let x = (0.1f32 * lane as f32) - 0.45;
  //   self.instantiate(t, x, -1.0)
  // }
  /// Destroy the object instance with the given ID.
  pub fn destroy(&mut self, id: u32) {
    let Some(object) = self.objects.map.values_mut().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    object.instances.remove(&id);
    info!("destroyed object instance (id: {})", id);
  }
  /// Set the x and y velocity of the object instance with the given ID.
  pub fn velocity(&mut self, id: u32, x: f32, y: f32) {
    let instance = self.get_instance_mut(id);
    instance.velocity = Vector2 { x, y };
    // info!("set {} instance velocity (pps) (id: {}, x: {}, y: {})", instance.t, id, x, y);
  }
  /// Get a reference to the object instance with the given ID.
  fn get_instance(&self, id: u32) -> &Instance {
    let Some(object) = self.objects.map.values().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    let Some(instance) = object.instances.get(&id) else { todo!(); };
    instance
  }
  /// Get a mutable reference to the object instance with the given ID.
  fn get_instance_mut(&mut self, id: u32) -> &mut Instance {
    let Some(object) = self.objects.map.values_mut().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    let Some(instance) = object.instances.get_mut(&id) else { todo!(); };
    instance
  }
}

pub fn load_binary(filename: &str) -> anyhow::Result<Vec<u8>> {
  let path = std::path::Path::new(env!("OUT_DIR"))
    .join("assets")
    .join(filename);
  let data = std::fs::read(path)?;
  Ok(data)
}

fn read_lines<P>(filename: P) -> io::Result<io::Lines<io::BufReader<File>>>
where
  P: AsRef<Path>,
{
  let file = File::open(filename)?;
  Ok(io::BufReader::new(file).lines())
}

/// Convert an integer width and height into a number between zero and two.
pub fn zero_to_two(width: u16, height: u16, window_size: PhysicalSize<u32>) -> (f32, f32) {
  let x_pixel = 2.0 / window_size.width as f32;
  let y_pixel = 2.0 / window_size.height as f32;
  let width = width as f32 * x_pixel;
  let height = height as f32 * y_pixel;
  (width, height)
}
