# Testing with artificial load

## Prerequisites 
```pip install requests```

## How to run

```python simulate_load.py -s [scenario]```

## Available scenarios:

|Flag |	Description|
|------|-----------|
|noisy |	Only one client sends many concurrent requests (simulates abuse)|
|polite |	Many clients each send a moderate number of requests (normal usage)|
|both |Polite clients run alongside a noisy neighbor (tests fairness)|
