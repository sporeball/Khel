use crate::{load_binary, texture::{self, DrawTexture, Texture}, Instance};
use std::collections::HashMap;
use std::time::Duration;
use cgmath::Vector3;
use log::info;
use wgpu::{util::{BufferInitDescriptor, DeviceExt}, Buffer, BufferUsages, Device, Queue, RenderPass};
use winit::dpi::PhysicalSize;

pub struct Object {
  pub texture: Texture,
  pub instances: HashMap<u32, Instance>,
  pub instance_buffer: Buffer,
}

impl Object {
  pub fn from_file(filename: &str, window_size: PhysicalSize<u32>, device: &Device, queue: &Queue) -> Result<Self, anyhow::Error> {
    let bytes = load_binary(filename).unwrap();
    let texture = texture::Texture::from_bytes(window_size, device, queue, &bytes, filename).unwrap();
    // let instances = vec![];
    let instances: HashMap<u32, Instance> = HashMap::new();
    let instance_buffer = create_instance_buffer(&instances, device);
    Ok(Self {
      texture,
      instances,
      instance_buffer,
    })
  }
}

#[derive(Default)]
/// A collection of all types of game objects created by Khel.
pub struct Objects {
  pub map: HashMap<String, Object>,
  pub min_available_id: u32,
  // TODO
  pub to_be_destroyed: HashMap<u32, Duration>,
}

impl Objects {
  /// Instantiate an object at the given coordinates.
  /// Returns the ID of the object instance.
  pub fn instantiate(
    &mut self,
    t: &str,
    x: f32,
    y: f32,
    window_size: PhysicalSize<u32>,
    device: &Device,
    queue: &Queue,
  ) -> u32 {
    // create an entry in objects if none exists
    if !self.map.contains_key(t) {
      let filename = format!("{}.png", t);
      let object = Object::from_file(
        filename.as_str(),
        window_size,
        device,
        queue,
      ).unwrap();
      self.map.insert(t.to_string(), object);
    }
    // create an instance
    let object = self.map.get_mut(t).unwrap();
    let id = self.min_available_id;
    let instance = Instance {
      // t: t.to_string(),
      position: Vector3 { x, y, z: 0.0 },
      // create_time: self.time,
      // destroy_time: Duration::MAX,
    };
    // push the instance
    object.instances.insert(id, instance.clone());
    object.instance_buffer = create_instance_buffer(&object.instances, device);
    // info!("created {} instance (id: {})", t, id);
    self.min_available_id += 1;
    id
  }
  /// Destroy the object instance with the given ID.
  pub fn destroy(&mut self, id: u32) {
    let Some(object) = self.map.values_mut().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    object.instances.remove(&id);
    info!("destroyed object instance (id: {})", id);
  }
  /// Get a reference to the object instance with the given ID.
  pub fn get_instance(&self, id: u32) -> &Instance {
    let Some(object) = self.map.values().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    let Some(instance) = object.instances.get(&id) else { todo!(); };
    instance
  }
  /// Get a mutable reference to the object instance with the given ID.
  pub fn get_instance_mut(&mut self, id: u32) -> &mut Instance {
    let Some(object) = self.map.values_mut().find(|o| o.instances.contains_key(&id)) else { todo!(); };
    let Some(instance) = object.instances.get_mut(&id) else { todo!(); };
    instance
  }
}

#[derive(Default)]
/// A collection of object IDs, able to be manipulated together.
pub struct Group {
  pub vec: Vec<u32>,
}

impl Group {
  /// Add the object instance with the given ID to this group.
  pub fn insert(&mut self, id: u32) {
    self.vec.push(id);
  }
  /// Remove the object instance with the given ID from this group.
  pub fn remove(&mut self, id: u32) {
    self.vec.retain(|&x| x != id);
  }
  /// Return the number of object IDs in this group.
  pub fn size(&self) -> usize {
    self.vec.len()
  }
  /// Call a function on every object ID in the group.
  pub fn for_each_id<F: Fn(u32)>(&mut self, f: F) {
    for id in self.vec.iter() {
      f(*id);
    }
  }
  /// Call a function on every instance in the group.
  pub fn for_each_instance<F: Fn(&mut Instance)>(&mut self, f: F, objects: &mut Objects) {
    for id in self.vec.iter() {
      let instance = objects.get_instance_mut(*id);
      f(instance);
    }
  }
  /// Call a function on every instance in the group, including an index `i`.
  pub fn for_each_instance_enumerated<F: Fn(usize, &mut Instance)>(&mut self, f: F, objects: &mut Objects) {
    for (index, id) in self.vec.iter().enumerate() {
      let instance = objects.get_instance_mut(*id);
      f(index, instance);
    }
  }
}

#[derive(Default)]
pub struct Groups {
  pub map: HashMap<String, Group>,
}

impl Groups {
  /// Create a new group.
  pub fn insert(&mut self, name: String, vec: Vec<u32>) {
    let group = Group { vec };
    self.map.insert(name, group);
  }
  /// Add the instance with the given ID to the group with the given name.
  pub fn insert_into_group(&mut self, name: String, id: u32) {
    self.get_mut(name).insert(id);
  }
  /// Remove the instance with the given ID from the group with the given name.
  pub fn remove_from_group(&mut self, name: String, id: u32) {
    self.get_mut(name).remove(id);
  }
  /// Get a reference to the group with the given name.
  pub fn get(&self, name: String) -> &Group {
    let Some(group) = self.map.get(&name) else { todo!(); };
    group
  }
  /// Get a mutable reference to the group with the given name.
  pub fn get_mut(&mut self, name: String) -> &mut Group {
    let Some(group) = self.map.get_mut(&name) else { todo!(); };
    group
  }
}

// TODO: is it possible to put this on Object? i forgot...
// pub fn create_instance_buffer(instances: &Vec<Instance>, device: &Device) -> Buffer {
pub fn create_instance_buffer(instances: &HashMap<u32, Instance>, device: &Device) -> Buffer {
  // let instance_data = instances
  //   .iter()
  //   .map(Instance::to_raw)
  //   .collect::<Vec<_>>();
  let instance_data = instances
    .values()
    .cloned()
    .collect::<Vec<_>>()
    .iter()
    .map(Instance::to_raw)
    .collect::<Vec<_>>();
  let instance_buffer = device.create_buffer_init(&BufferInitDescriptor {
    label: Some("Instance Buffer"),
    contents: bytemuck::cast_slice(&instance_data),
    usage: BufferUsages::VERTEX,
  });
  instance_buffer
}

pub trait DrawObject<'a> {
  fn draw_object_instanced(&mut self, object: &'a Object);
}

impl<'a, 'b> DrawObject<'b> for RenderPass<'a>
where
  'b: 'a,
{
  fn draw_object_instanced(&mut self, object: &'b Object) {
    self.set_vertex_buffer(1, object.instance_buffer.slice(..));
    self.draw_texture_instanced(&object.texture, 0..object.instances.len() as u32);
  }
}
