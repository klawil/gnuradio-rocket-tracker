CREATE TABLE public.packets (
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    packet_json jsonb
);
