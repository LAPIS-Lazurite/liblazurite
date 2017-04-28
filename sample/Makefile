All: tx64 tx raw rx link  promiscuous

tx:
	g++ -I./ -o sample_tx sample_tx.cpp -L/usr/lib -llazurite

tx64:
	g++ -I./ -o sample_tx64 sample_tx64.cpp -L/usr/lib -llazurite

raw:
	g++ -I./ -o sample_rx_raw sample_rx_raw.cpp -L/usr/lib -llazurite

rx:
	g++ -I./ -o sample_rx_payload sample_rx_payload.cpp -L/usr/lib -llazurite

link:
	g++ -I./ -o sample_rx_link sample_rx_link.cpp -L/usr/lib -llazurite

promiscuous:
	g++ -I./ -o sample_rx_promiscuous sample_rx_promiscuous.cpp -L/usr/lib -llazurite

clean:
	rm sample_tx sample_rx_raw sample_rx_payload sample_rx_link sample_tx64 sample_rx_promiscuous
