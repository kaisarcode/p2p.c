# HPM Manifesto

## Cyberspace as a Neighborhood Fair

### A network does not have to be a platform

For years, it has become normal to think that in order to communicate, publish something, or participate in a community, we must first register on a platform.

- Create an account.
- Provide an email address.
- Accept terms and conditions.
- Choose a password.
- Verify an identity.
- Be measured.
- Be classified.
- Be recommended or made invisible by an algorithm.

HPM begins with a different idea:

> **Not every relationship needs a platform in the middle.**

Some bonds already exist before technology: friendships, families, neighborhoods, cooperatives, businesses, clubs, work groups, and small communities. A network should be able to serve those bonds without taking ownership of them.

HPM does not seek to create a community of users. It seeks to provide tools so that real communities can build and share their own network.

That is why this manifesto speaks of **networks**, **computer networks**, and **cyberspace**. The web may be part of that space, but it does not exhaust it. HPM connects peers and services; HTTP and websites are only one possibility among many.

---

### We are not here to compete

HPM does not intend to replace the Internet.

It does not intend to displace social networks, messaging services, cloud providers, or global platforms. Nor does it seek to become the next universal network.

It exists because there is a different need.

Some people do not want to open a new account for everything. Some groups only need to share a server, a page, a radio station, a catalog, a game, or a tool among people they know. Some communities want to use their own devices without buying a domain, paying for hosting, or depending on a central company.

HPM does not claim that every network should work this way.

It makes a simpler claim:

> **This kind of network should also be allowed to exist.**

---

### The neighborhood fair

The best image for understanding this proposal is a fair.

You do not need to create an account to walk through a neighborhood fair. You simply arrive, look around, and see what is there.

- Some stalls are open.
- Others did not come today.
- Some will appear tomorrow.
- One person sells something.
- Another displays their work.
- Another plays music.
- Another invites people to play.
- Another shares information.

Nobody promises to be available twenty-four hours a day. What is there exists because someone chose to sustain it at that moment.

The network we imagine looks like that.

A service may appear and disappear. It may exist only for an afternoon, during a meeting, on weekends, or while a computer remains turned on. That temporariness is not necessarily a flaw. It is part of a human, nearby, and honest infrastructure.

> **Cyberspace as a neighborhood fair: open, changing, direct, and sustained by those who participate.**

---

### Publisher, index, and consumer

HPM provides a minimal system with three roles:

- a **publisher** publishes a service;
- an **index** helps others find it;
- a **consumer** connects to it.

The index is not a central platform, does not host applications, and does not normally carry their traffic. It acts as a meeting point: it registers names, introduces endpoints, and helps two devices attempt to communicate directly.

Then it steps aside.

These roles do not belong to special classes of infrastructure.

- Anyone can run an index.
- Anyone can publish.
- Anyone can consume.
- The same device can perform more than one role.

There is no mandatory official index and no single network that anyone must join.

---

### P2P means P2P

Direct communication is not a secondary optimization. It is one of the project's principles.

HPM was designed to establish P2P paths between participants. The index coordinates, but it does not become the permanent carrier of communication.

For that reason, TURN is outside the project's core.

**TURN is a relay system.** When two devices cannot connect directly, both send their data to a third server, which receives and forwards it. This can enable communication on restrictive networks, but it also means that traffic no longer travels directly between peers and instead depends on infrastructure that remains in the middle of the conversation.

TURN may be useful in other systems, and a derived application may implement it if needed. But making traffic relays a core responsibility of HPM would change its nature.

A relay must absorb bandwidth, maintain sessions, scale resources, and become a privileged point of observation and dependency. With enough growth, that model favors central infrastructures that only actors with significant resources can sustain.

HPM chooses a different boundary:

> **When a direct connection is not possible, the core may fail rather than stop being P2P.**

This is not an accidental omission. It is an ethical and architectural boundary.

---

### Edge computing for ordinary devices

The index is not intended to be a large central server.

It should be able to run on a home computer, a machine in a shop, a small server, a low-power board, or any reasonable device available to the community.

Work is distributed across the edges:

- peers process their own data;
- peers sustain their own services;
- HPM encrypts sessions by default in its reliable peer-to-peer transport layer;
- peers contribute their own bandwidth;
- the index performs only the necessary coordination.

This allows the network to grow without requiring any node to turn into an infrastructure company.

> **The network's capacity emerges from the sum of its participants, not from the growth of a center.**

---

### Scaling does not mean centralizing

HPM does not scale by creating an ever-larger index.

It scales through many small, independent, overlapping indexes.

- A publisher can register with several indexes.
- A consumer can query several indexes.
- An application can maintain its own list of known communities.

A service may be present in the index of a family, a group of friends, a cooperative, and a local business. Each community decides which indexes it uses and which ones it shares.

Indexes do not need to synchronize with one another or obey a global authority.

Growth happens through a fabric:

> **More communities, more indexes, and more links; not a more powerful server and a larger authority.**

---

### Word-of-mouth networks

Not every network needs global discovery.

The address of an index can be shared in a conversation, on paper, through a QR code, on a USB drive, in a file, or by any means the community considers appropriate.

Entering the network can begin with a simple sentence:

> “Use this index and look for this name.”

Trust does not have to come from a company that certifies every participant. It can begin with preexisting human relationships.

This favors small, understandable, and appropriable networks. Networks whose reach does not depend on becoming visible to the entire planet.

---

### No accounts by default

HPM does not ask for an email address.

- It does not create profiles.
- It does not require OAuth.
- It does not manage account recovery.
- It does not build a global identity.
- It does not need to know who a person is.

An identifier published in an index is not an account. It is an operational name that makes it possible to find a service within a specific context.

Participation does not depend on belonging to HPM because HPM is not a platform to belong to.

> **Using a tool should not require becoming a permanent user of a company.**

---

### Passwords without a user system

HPM passwords have a specific purpose.

`HPM_PASS` controls who may publish to an index and helps reduce abuse.

`HPM_VIP` reserves specific IDs so that someone else cannot take a name during an outage or disconnection of the legitimate publisher.

This is useful, for example, when a business publishes services such as:

- catalog
- orders
- radio
- reservations

If one of those services disconnects, the seat can remain protected so that another publisher cannot temporarily impersonate that name.

These passwords do not create accounts. They do not authenticate consumers. They do not define personal identities. They do not turn the index into an access provider.

They protect one small and concrete resource: the ability to publish and occupy certain names.

---

### Every stall is visible

Today's platforms often organize visibility through metrics, recommendations, advertising, and retention algorithms.

One piece of content appears because a company decided to show it. Another disappears because it does not generate enough interaction. Relationships are translated into followers, likes, reach, and performance.

The fair proposes something different.

The stalls are there. People walk through them and find them. They do not need to compete for attention before a machine that decides who deserves to be seen.

An application built on HPM could display services with:

- name;
- icon;
- description;
- category;
- status;
- community information.

It could visually resemble a server directory, an app store, or a notice board. But it would not have to become a market for visibility.

> **Being present should be enough to be found.**

---

### Community is not a metric

On many platforms, “community” means a mass of users gathered around infrastructure they do not control.

- The platform defines the rules.
- The platform keeps the data.
- The platform decides visibility.
- The platform can change the conditions.
- The platform can shut the space down.

That does not always constitute an autonomous community. It is often a rented audience.

As a result, publishers become constrained by the interests of the platform. It is no longer enough to create or share what they consider valuable: they must adapt to whatever produces retention, interaction, or profitability. When they do not, the system itself may demote, hide, or render their work irrelevant.

HPM begins with a different notion:

> **A community is a group capable of sustaining its own bonds and its own spaces.**

Technology should facilitate that autonomy, not replace it.

---

### Recovering a possibility of cyberspace

It is worth recovering a word that now seems old: **cyberspace**.

The term allows us to think of the network as a shared realm made up of places, bonds, services, and communities, rather than merely a collection of websites or commercial applications. HPM does not connect only pages: it connects peers capable of offering games, radio stations, files, tools, catalogs, interactive services, or any other application that can be built on a computer network.

The early Web was one of the most visible expressions of that idea. It was made of places: personal pages, forums, servers, directories, and spaces built by particular people or organizations. Not everything happened inside a few platforms.

But publishing was also difficult. It required knowledge, infrastructure, and resources that were not available to everyone. For a long time, networks were the territory of universities, large companies, and specialists.

Then mass adoption arrived, but so did concentration.

Today, material conditions have changed. Millions of people carry in their pockets computers that would have been unimaginable decades ago. Homes and small businesses have devices capable of publishing, processing, and storing information.

It is possible to rethink decentralized networks under different conditions.

Not to recreate the past exactly, but to recover one of its possibilities:

> **That anyone can have a place of their own on the network.**

Many other P2P proposals exist, each with its own goals and decisions. Some are primarily oriented toward file transfer; others were created around web technologies, anonymity, messaging, or specialized networks. HPM does not intend to replace or compete with them.

Its proposal is narrower: to provide a simple, general-purpose foundation so that a community can connect peers and build whatever it needs on top. It does not define in advance whether what is shared will be a page, a radio station, a game, a catalog, or a service not yet imagined.

---

### Publishing without buying presence

A person can run a Minecraft server at home and share it with friends without paying for hosting or buying a domain.

A business can publish a catalog from the computer in its shop.

A family can share files or tools.

A group can start a radio station.

A cooperative can offer internal services.

A neighborhood can have a notice board or a small community website.

The infrastructure remains in the hands of those who use it. There is no need to buy digital presence from an intermediary in order to exist before others.

---

### Limitations are also design

HPM is a small and deliberately limited library.

It does not include:

- global identity;
- profiles;
- followers;
- likes;
- rankings;
- mandatory telemetry;
- universal moderation;
- central storage;
- worldwide index synchronization;
- permanent availability;
- mandatory relays;
- an official authority.

These absences should not automatically be interpreted as errors or missing features.

Some capabilities, when added to the core, create centers of control, infrastructure requirements, and possibilities for concentration.

Limitations can protect a scale.

> **Not every tool must be designed to become a monopoly.**

---

### A foundation, not a total application

HPM solves the minimal publisher, index, and consumer system.

Higher layers can add:

- lists of indexes;
- visual catalogs;
- metadata;
- descriptions;
- icons;
- community protocols;
- permissions;
- local identity;
- invitation systems;
- external relay mechanisms;
- specific experiences for games, radio stations, pages, or businesses.

Each application decides how much to build and according to which values.

The core does not attempt to anticipate every possibility or impose a single form of community.

> **HPM provides tools; communities build their spaces.**

---

### Freedom to create and freedom to leave

Community infrastructure should allow its participants to:

- run their own index;
- change indexes;
- publish to several;
- stop publishing;
- create another network;
- share tools;
- modify the software;
- avoid depending on an official instance.

No node should be irreplaceable.

No community should be trapped because a company owns its identity, history, or access.

Freedom is not only the ability to enter. It is also the ability to leave, copy, replace, and start again.

---

### We do not seek captive users

HPM does not need to grow indefinitely in order to justify its existence.

- A network used by five friends can be successful.
- A family network can be enough.
- An installation for a business can fulfill its purpose.
- A small community is not an incomplete stage on the way to a massive platform.

Value is not measured by number of accounts, time spent, or market share.

> **A tool can matter without conquering the world.**

---

### Our proposal

This is not the only possible way to build decentralized networks.

It does not claim to be the final or universal answer. Many projects, protocols, and communities explore different paths.

HPM is a concrete proposal:

- P2P communication;
- minimal infrastructure;
- small indexes;
- participation without accounts;
- publishing from ordinary devices;
- ephemeral networks;
- word-of-mouth discovery;
- applications built on top;
- communities before platforms;
- deliberate limits against concentration.

We do not claim that every network should be a fair.

We claim that there should also be room for fairs.

---

## Principles

### The network should serve the community

The community should not become raw material for a platform.

### Publishing should not require permission

Anyone with a device and a connection should be able to share a service.

### Participation should not require an account

Not every interaction needs a global identity, email address, password, or OAuth.

### The index coordinates; it does not govern

It helps peers find one another and then steps away from the communication.

### The data path should be direct

P2P is a core principle, not a cosmetic option.

### Scale should come from multiplication

Many small indexes are preferable to one mandatory center.

### Temporariness is legitimate

A service may be here today and gone tomorrow. That is also a network.

### Visibility should not depend on metrics

Stalls exist to be walked through, not to compete before an algorithm.

### Limitations can protect freedom

Not every technical capability is desirable within the core.

### Cyberspace is not only the web

Pages and websites are one possibility, not the limit. The network can also carry games, radio stations, files, tools, catalogs, conversations, and services not yet imagined.

### No one should own the entire network

Every person and every community should be able to run, modify, and replace its own pieces.

---

## Closing

HPM does not want to build a new central platform.

It wants to shorten the path between a person who has something to share and a community that wants to find it.

- Without unnecessary accounts.
- Without mandatory domains.
- Without hosting as a requirement.
- Without inherent telemetry.
- Without visibility algorithms.
- Without an indispensable central authority.

- One person opens a stall.
- Another walks by and looks.
- Someone recommends something.
- A service appears.
- Another shuts down.
- The community changes.

Nothing promises to be global. Nothing needs to be permanent. Nothing has to become a monopoly in order to have value.

> **HPM proposes the network as a common space: direct, small, free, and deeply human.**
