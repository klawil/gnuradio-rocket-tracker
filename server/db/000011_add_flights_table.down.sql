DROP TRIGGER IF EXISTS packet_flight_state ON public.packets;
DROP TABLE IF EXISTS public.device_flights;
DROP FUNCTION IF EXISTS public.sync_packets_to_flights();
