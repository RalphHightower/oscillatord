#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "oscillator_factory.h"
#include "config.h"
#include "log.h"

#ifndef MAX_OSCILLATOR_FACTORIES
#define MAX_OSCILLATOR_FACTORIES 5
#endif

static const struct oscillator_factory * factories[MAX_OSCILLATOR_FACTORIES];
static unsigned factories_nb;

static const struct oscillator_factory *oscillator_factory_get_by_name(
		const char *name)
{
	unsigned i;

	for (i = 0; i < factories_nb; i++)
		if (strcmp(name, factories[i]->name) == 0)
			return factories[i];

	errno = ENOENT;

	return NULL;
}

struct oscillator *oscillator_factory_new(const struct config *config)
{
	int ret;
	const char *name;
	const struct oscillator_factory *factory;

	name = config_get(config, "oscillator");
	if (name == NULL) {
		ret = errno;
		err("Configuration \"%s\" doesn't have an oscillator entry.\n",
				config->path);
		errno = ret;
		return NULL;
	}

	factory = oscillator_factory_get_by_name(name);
	if (factory == NULL) {
		ret = errno;
		err("Display type \"%s\" unknown, check configuration %s\n",
				name, config->path);
		errno = ret;
		return NULL;
	}

	return factory->new(config);
}

static bool oscillator_factory_is_valid(const struct oscillator_factory *factory)
{
	return factory != NULL && factory->destroy != NULL &&
			factory->name != NULL && *factory->name != '\0' &&
			factory->new != NULL;
}

int oscillator_factory_register(const struct oscillator_factory *factory)
{
	if (!oscillator_factory_is_valid(factory))
		return -EINVAL;

	debug("%s(%s)\n", __func__, factory->name);

	if (factories_nb == MAX_OSCILLATOR_FACTORIES) {
		err("no room left for factories, see "
				"MAX_OSCILLATOR_FACTORIES\n");
		return -ENOMEM;
	}
	factories[factories_nb] = factory;
	factories_nb++;

	return 0;
}

void oscillator_factory_destroy(struct oscillator **oscillator)
{
	const struct oscillator_factory *factory;

	if (oscillator == NULL || *oscillator == NULL)
		return;
	factory = oscillator_factory_get_by_name((*oscillator)->factory_name);
	if (factory == NULL)
		return;

	factory->destroy(oscillator);
	*oscillator = NULL;
}