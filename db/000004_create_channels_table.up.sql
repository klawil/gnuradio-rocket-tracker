CREATE TABLE public.channels (
  source_id character varying(20) NOT NULL,
  created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
  deleted_at timestamp(6) without time zone,
  frequency numeric(8, 4) NOT NULL,
  channel_name character varying(50)
);
ALTER TABLE public.channels OWNER TO william;
GRANT ALL ON TABLE public.channels TO rockettracker;
GRANT ALL ON TABLE public.channels TO william;
