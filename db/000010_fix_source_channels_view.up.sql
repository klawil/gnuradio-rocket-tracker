CREATE OR REPLACE VIEW public.source_channels AS SELECT DISTINCT ON (channels.frequency, channel_names.frequency)
  channels.created_at,
  channels.deleted_at,
  CASE
  	WHEN channels.frequency IS NULL THEN channel_names.frequency
	ELSE channels.frequency
  END as frequency,
  channels.source_id,
  channel_names.channel_name
FROM public.channel_names
  FULL OUTER JOIN public.channels ON channel_names.frequency = channels.frequency
ORDER BY channels.frequency, channel_names.frequency, channels.deleted_at DESC, channels.created_at DESC;
