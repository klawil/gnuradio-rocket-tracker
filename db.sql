SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

-- Table: packets
DROP TABLE IF EXISTS public.packets CASCADE;
CREATE TABLE public.packets (
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    packet_json jsonb
);
ALTER TABLE public.packets OWNER TO william;
GRANT ALL ON TABLE public.packets TO rockettracker;
GRANT ALL ON TABLE public.packets TO william;

-- Table: devices
DROP TABLE IF EXISTS public.devices CASCADE;
CREATE TABLE public.devices (
    serial integer NOT NULL,
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    name character varying(100),
    device_type smallint
);
ALTER TABLE public.devices OWNER TO william;
ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_serial PRIMARY KEY (serial);
GRANT ALL ON TABLE public.devices TO rockettracker;
GRANT ALL ON TABLE public.devices TO william;

-- Most recent rocket state view
DROP VIEW IF EXISTS public.devices_state;
CREATE VIEW public.devices_state AS SELECT
    devices.name,
    devices.device_type,
    packets_parsed.*
FROM (
    SELECT DISTINCT ON (packet_json->'type', packet_json->'serial')
        created_at,
        CAST(packet_json->'type' AS smallint) AS "type",
        CAST(packet_json->'serial' AS integer) AS serial,
        packet_json
    FROM public.packets
    ORDER BY packet_json->'type', packet_json->'serial', packet_json->'rtime' DESC, packet_json->'time' DESC
) packets_parsed
LEFT JOIN public.devices ON packets_parsed.serial = devices.serial;
ALTER VIEW public.devices_state OWNER TO william;
GRANT ALL ON public.devices_state TO rockettracker;
GRANT ALL ON public.devices_state TO william;
