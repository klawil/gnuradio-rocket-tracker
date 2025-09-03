CREATE OR REPLACE VIEW public.devices_state AS SELECT
	devices.name as device_name,
	current_state.device_serial,
	current_state.max_created_at,
	current_state.min_created_at,
	current_state.combined_state
FROM (
	SELECT
		device_serial,
		MAX(created_at) as max_created_at,
		MIN(CASE WHEN first_packet IS NULL THEN created_at ELSE first_packet END) as min_created_at,
		jsonb_object_agg(packet_type, packet_json) as combined_state
	FROM (
	SELECT DISTINCT ON (packet_json->'Serial', packet_json->'Type')
		CAST(pa.packet_json->'Serial' as integer) as device_serial,
		CAST(pa.packet_json->'Type' as smallint) as packet_type,
		pa.packet_json,
		pa.created_at,
		df.first_packet
	FROM public.packets pa
	LEFT JOIN (
		SELECT DISTINCT ON (serial)
			serial,
			flight,
			first_packet
		FROM public.device_flights
		ORDER BY serial, last_packet DESC
	) df
		ON df.serial = CAST(pa.packet_json->'Serial' as integer)
			AND df.first_packet <= pa.created_at
	ORDER BY pa.packet_json->'Serial', pa.packet_json->'Type', pa.packet_json->'Time' DESC
	) agg
	GROUP BY agg.device_serial
) current_state
	LEFT JOIN public.devices ON current_state.device_serial = devices.serial;