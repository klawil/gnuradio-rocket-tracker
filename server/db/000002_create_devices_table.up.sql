CREATE TABLE public.devices (
    serial integer NOT NULL,
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    name character varying(100),
    device_type smallint
);
ALTER TABLE ONLY public.devices DROP CONSTRAINT IF EXISTS devices_serial;
ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_serial PRIMARY KEY (serial);
