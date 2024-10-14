use crate::{AutoVelocity, chart::BpmList};

#[test]
fn autovelocity_over_time() -> Result<(), anyhow::Error> {
  let av = AutoVelocity { value: 300.0 };
  let bpm_list = BpmList::from_string("144@0.0,72@32.0,144@48.0".to_string())?;
  assert_eq!(av.over_time(0.0, &bpm_list), 0.0);
  assert_eq!(av.over_time(2.0, &bpm_list), 600.0);
  assert_eq!(av.over_time((40.0 / 3.0) + 2.0, &bpm_list), 4000.0 + 300.0);
  assert_eq!(av.over_time((80.0 / 3.0) + 2.0, &bpm_list), 4000.0 + 2000.0 + 600.0);
  Ok(())
}

// #[test]
// fn pure_calculation() -> Result<(), anyhow::Error> {
//   Ok(())
// }
