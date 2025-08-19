CREATE OR REPLACE VIEW public.source_channels AS SELECT DISTINCT ON (channels.frequency)
  channels.created_at,
  channels.deleted_at,
  channels.frequency,
  channels.source_id,
  channel_names.channel_name
FROM public.channel_names
  FULL OUTER JOIN public.channels ON channel_names.frequency = channels.frequency
ORDER BY channels.frequency, channels.deleted_at DESC, channels.created_at DESC;
