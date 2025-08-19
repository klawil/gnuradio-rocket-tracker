CREATE TABLE public.channel_names (
  frequency integer NOT NULL,
  channel_name character varying(50)
);
ALTER TABLE ONLY public.channel_names DROP CONSTRAINT IF EXISTS channel_names_key;
ALTER TABLE ONLY public.channel_names
  ADD CONSTRAINT channel_names_key PRIMARY KEY (frequency);

CREATE OR REPLACE VIEW public.source_channels AS SELECT DISTINCT ON (channels.frequency)
  channels.created_at,
  channels.deleted_at,
  channels.frequency,
  channels.source_id,
  channel_names.channel_name
FROM public.channel_names
  FULL OUTER JOIN public.channels ON channel_names.frequency = channels.frequency
ORDER BY channels.frequency, channels.deleted_at DESC, channels.created_at DESC;