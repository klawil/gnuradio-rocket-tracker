CREATE TABLE public.packets (
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    packet_json jsonb
);
ALTER TABLE public.packets OWNER TO william;
GRANT ALL ON TABLE public.packets TO rockettracker;
GRANT ALL ON TABLE public.packets TO william;
