use anyhow::*;
use image::{DynamicImage, GenericImageView};
// use log::info;
use wgpu::{util::{BufferInitDescriptor, DeviceExt}, AddressMode, BindGroup, BindGroupDescriptor, BindGroupEntry, BindGroupLayout, BindGroupLayoutDescriptor, BindGroupLayoutEntry, BindingResource, BindingType, Buffer, BufferUsages, Device, Extent3d, FilterMode, ImageCopyTexture, ImageDataLayout, IndexFormat, Origin3d, Queue, RenderPass, SamplerBindingType, SamplerDescriptor, ShaderStages, TextureAspect, TextureDescriptor, TextureDimension, TextureFormat, TextureSampleType, TextureUsages, TextureViewDescriptor, TextureViewDimension};
use winit::dpi::PhysicalSize;

use crate::Vertex;

pub struct Texture {
  pub texture: wgpu::Texture,
  // pub view: TextureView,
  // pub sampler: Sampler,
  pub bind_group: BindGroup,
  pub bind_group_layout: BindGroupLayout,
  pub vertex_buffer: Buffer,
  pub index_buffer: Buffer,
}

impl Texture {
  pub fn from_bytes(window_size: PhysicalSize<u32>, device: &Device, queue: &Queue, bytes: &[u8], label: &str) -> Result<Self> {
    let img = image::load_from_memory(bytes)?;
    Self::from_image(window_size, device, queue, &img, Some(label))
  }
  pub fn from_image(window_size: PhysicalSize<u32>, device: &Device, queue: &Queue, img: &DynamicImage, label: Option<&str>) -> Result<Self> {
    let rgba = img.to_rgba8();
    let dimensions = img.dimensions();
    let size = Extent3d {
      width: dimensions.0,
      height: dimensions.1,
      depth_or_array_layers: 1,
    };
    let texture = device.create_texture(&TextureDescriptor {
      label,
      size,
      mip_level_count: 1,
      sample_count: 1,
      dimension: TextureDimension::D2,
      format: TextureFormat::Rgba8UnormSrgb,
      usage: TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_DST,
      view_formats: &[],
    });
    queue.write_texture(
      ImageCopyTexture {
        aspect: TextureAspect::All,
        texture: &texture,
        mip_level: 0,
        origin: Origin3d::ZERO,
      },
      &rgba,
      ImageDataLayout {
        offset: 0,
        bytes_per_row: Some(4 * dimensions.0),
        rows_per_image: Some(dimensions.1),
      },
      size,
    );
    let view = texture.create_view(&TextureViewDescriptor::default());
    let sampler = device.create_sampler(&SamplerDescriptor {
      address_mode_u: AddressMode::ClampToEdge,
      address_mode_v: AddressMode::ClampToEdge,
      address_mode_w: AddressMode::ClampToEdge,
      mag_filter: FilterMode::Linear,
      min_filter: FilterMode::Nearest,
      mipmap_filter: FilterMode::Nearest,
      ..Default::default()
    });
    let bind_group_layout = device.create_bind_group_layout(&bgl_desc());
    let bind_group = device.create_bind_group(&BindGroupDescriptor {
      layout: &bind_group_layout,
      entries: &[
        BindGroupEntry {
          binding: 0,
          resource: BindingResource::TextureView(&view),
        },
        BindGroupEntry {
          binding: 1,
          resource: BindingResource::Sampler(&sampler),
        },
      ],
      label: Some("diffuse_bind_group"),
    });
    let vertex_buffer = create_vertex_buffer(size, window_size, device);
    // let (w2, h2) = self.size_zero_to_two(window_size);
    // info!("(w2, h2) = ({w2}, {h2})");
    let indices = vec![
      0, 1, 3,
      1, 2, 3,
    ];
    let index_buffer = device.create_buffer_init(&BufferInitDescriptor {
      label: Some("Index Buffer"),
      contents: bytemuck::cast_slice(&indices),
      usage: BufferUsages::INDEX,
    });
    Ok(Self {
      texture,
      // view,
      // sampler,
      bind_group_layout,
      bind_group,
      vertex_buffer,
      index_buffer,
    })
  }
}

pub fn bgl_desc() -> BindGroupLayoutDescriptor<'static> {
  BindGroupLayoutDescriptor {
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
  }
}

pub fn create_vertex_buffer(size: Extent3d, window_size: PhysicalSize<u32>, device: &Device) -> Buffer {
  let w = (size.width as f32 / (window_size.width as f32 / 2.0) - 1.0) * (size.width as f32 / window_size.width as f32);
  let h = (size.height as f32 / (window_size.height as f32 / 2.0) - 1.0) * (size.height as f32 / window_size.height as f32);
  // info!("(w, h) = ({}, {})", w, h);
  let vertices = vec![
    Vertex { position: [-w, h, 0.0], tex_coords: [0.0, 1.0]}, // top left
    Vertex { position: [-w, -h, 0.0], tex_coords: [0.0, 0.0]}, // bottom left
    Vertex { position: [w, -h, 0.0], tex_coords: [1.0, 0.0]}, // bottom right
    Vertex { position: [w, h, 0.0], tex_coords: [1.0, 1.0]}, // top right
  ];
  let vertex_buffer = device.create_buffer_init(&BufferInitDescriptor {
    label: Some("Vertex Buffer"),
    contents: bytemuck::cast_slice(&vertices),
    usage: BufferUsages::VERTEX,
  });
  vertex_buffer
}

pub trait DrawTexture<'a> {
  fn draw_texture(&mut self, texture: &'a Texture);
  fn draw_texture_instanced(&mut self, texture: &'a Texture, instances: core::ops::Range<u32>);
}

impl<'a, 'b> DrawTexture<'b> for RenderPass<'a>
where
  'b: 'a,
{
  fn draw_texture(&mut self, texture: &'b Texture) {
    self.draw_texture_instanced(texture, 0..1);
  }
  fn draw_texture_instanced(&mut self, texture: &'b Texture, instances: core::ops::Range<u32>) {
    self.set_bind_group(0, &texture.bind_group, &[]);
    self.set_vertex_buffer(0, texture.vertex_buffer.slice(..));
    self.set_index_buffer(texture.index_buffer.slice(..), IndexFormat::Uint32);
    self.draw_indexed(0..6, 0, instances);
  }
}

// pub trait ZeroToTwo {
//   fn zero_to_two(&self, window_size: PhysicalSize<u32>) -> (f32, f32);
// }

// impl ZeroToTwo for Extent3d {
//   fn zero_to_two(&self, window_size: PhysicalSize<u32>) -> (f32, f32) {
//     let x_pixel = 2.0 / window_size.width as f32;
//     let y_pixel = 2.0 / window_size.height as f32;
//     let width = self.width as f32 * x_pixel;
//     let height = self.height as f32 * y_pixel;
//     (width, height)
//   }
// }
