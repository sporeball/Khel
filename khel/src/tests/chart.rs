use crate::chart::{Bpm, BpmList};

#[test]
fn bpm_length() -> Result<(), anyhow::Error> {
  let bpm1 = Bpm::from_string("144@0.0".to_string())?;
  let bpm2 = Bpm::from_string("72@32.0".to_string())?;
  let bpm3 = Bpm::from_string("144@48.0".to_string())?;
  let bpm1_length = bpm1.length(Some(&bpm2));
  let bpm2_length = bpm2.length(Some(&bpm3));
  let bpm3_length = bpm3.length(None);
  let one_bar_144bpm = 5.0 / 3.0;
  let one_beat_144bpm = one_bar_144bpm / 4.0;
  let one_beat_72bpm = one_beat_144bpm * 2.0;
  assert_eq!(bpm1_length, one_beat_144bpm * 32.0);
  assert_eq!(bpm2_length, one_beat_72bpm * 16.0);
  assert_eq!(bpm3_length, f64::MAX);
  Ok(())
}

#[test]
fn bpmlist_at_exact_time() -> Result<(), anyhow::Error> {
  let bpm_list = BpmList::from_string("144@0.0,72@32.0,144@48.0".to_string())?;
  assert_eq!(bpm_list.at_exact_time(-10.0 / 3.0), &bpm_list.0[0]);
  assert_eq!(bpm_list.at_exact_time(0.0), &bpm_list.0[0]);
  assert_eq!(bpm_list.at_exact_time(5.0), &bpm_list.0[0]);
  assert_eq!(bpm_list.at_exact_time(40.0 / 3.0), &bpm_list.0[1]);
  assert_eq!(bpm_list.at_exact_time(15.0), &bpm_list.0[1]);
  assert_eq!(bpm_list.at_exact_time(80.0 / 3.0), &bpm_list.0[2]);
  assert_eq!(bpm_list.at_exact_time(30.0), &bpm_list.0[2]);
  Ok(())
}
