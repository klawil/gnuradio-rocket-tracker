CREATE OR REPLACE VIEW public.devices_state AS SELECT
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
